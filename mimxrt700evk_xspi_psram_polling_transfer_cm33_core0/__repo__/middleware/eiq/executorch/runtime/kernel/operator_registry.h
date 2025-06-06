/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cstring>

#include <executorch/runtime/core/array_ref.h>
#include <executorch/runtime/core/error.h>
#include <executorch/runtime/core/evalue.h>
#include <executorch/runtime/core/exec_aten/exec_aten.h>
#include <executorch/runtime/core/result.h>
#include <executorch/runtime/core/span.h>
#include <executorch/runtime/platform/compiler.h>
#include <executorch/runtime/platform/platform.h>

// Debug switch for operator registry
#if defined(ET_OP_REGISTRY_DEBUG)
#include <ostream>
#endif

#define ET_LOG_KERNEL_KEY(k)      \
  ET_LOG(                         \
      Error,                      \
      "key: %s, is_fallback: %s", \
      k.data(),                   \
      k.is_fallback() ? "true" : "false");
#define ET_LOG_TENSOR_META(meta_list)                                 \
  for (const auto& meta : meta_list) {                                \
    ET_LOG(Error, "dtype: %d | dim order: [", int(meta.dtype_));      \
    for (int i = 0; i < meta.dim_order_.size(); i++) {                \
      ET_LOG(Error, "%d,", static_cast<int32_t>(meta.dim_order_[i])); \
    }                                                                 \
    ET_LOG(Error, "]");                                               \
  }

namespace executorch {
namespace runtime {

class KernelRuntimeContext; // Forward declaration
using OpFunction = void (*)(KernelRuntimeContext&, EValue**);

/**
 * Dtype and dim order metadata for a Tensor argument to an operator.
 * Used by the Executor to hold the tensor metadata info and retrieve kernel.
 */
struct TensorMeta {
  executorch::aten::ScalarType dtype_;
  Span<executorch::aten::DimOrderType> dim_order_;

  TensorMeta() = default;
  TensorMeta(
      executorch::aten::ScalarType dtype,
      Span<executorch::aten::DimOrderType> order)
      : dtype_(dtype), dim_order_(order) {}

  bool operator==(const TensorMeta& other) const {
    return this->equals(other);
  }

  bool operator!=(const TensorMeta& other) const {
    return !this->equals(other);
  }

  bool equals(const TensorMeta& other) const {
    if (dtype_ != other.dtype_) {
      return false;
    }
    if (dim_order_.size() != other.dim_order_.size()) {
      return false;
    }
    for (int i = 0; i < dim_order_.size(); i++) {
      if (dim_order_[i] != other.dim_order_[i]) {
        return false;
      }
    }
    return true;
  }

#if defined(ET_OP_REGISTRY_DEBUG)
  friend std::ostream& operator<<(std::ostream& os, const TensorMeta& meta) {
    os << "dtype: " << int(meta.dtype_) << " | dim order: [";
    for (int i = 0; i < meta.dim_order_.size(); i++) {
      os << static_cast<int32_t>(meta.dim_order_[i]) << ", ";
    }
    os << "]";
    return os;
  }
#endif
};

/**
 * Describes which dtype & dim order specialized kernel to be bound to an
 * operator. If `is_fallback_` is true, it means this kernel can be used as a
 * fallback, if false, it means this kernel can only be used if all the
 * `TensorMeta` are matched. Fallback means this kernel will be used for
 * all input tensor dtypes and dim orders, if the specialized kernel is not
 * registered.
 *
 * The format of a kernel key data is a string:
 *                              "v<version>/<tensor_meta>|<tensor_meta>..."
 * Size: Up to 691               1    1    1     (42     +1) * 16
 *           Assuming max number of tensors is 16               ^
 * Kernel key version is v1 for now. If the kernel key format changes,
 * update the version to avoid breaking pre-existing kernel keys.
 * Example: v1/7;0,1,2,3
 * The kernel key has only one tensor: a double tensor with dimension 0, 1, 2, 3
 *
 * Each tensor_meta has the following format: "<dtype>;<dim_order,...>"
 * Size: Up to 42                               1-2   1    24 (1 byte for 0-9; 2
 * for 10-15) + 15 commas Assuming that the max number of dims is 16 ^ Example:
 * 7;0,1,2,3 for [double; 0, 1, 2, 3]
 *
 * IMPORTANT:
 * Users should not construct a kernel key manually. Instead, it should be
 * generated from kernel yaml.
 */
struct KernelKey {
 public:
  KernelKey() : is_fallback_(true) {}

  /* implicit */ KernelKey(const char* kernel_key_data)
      : kernel_key_data_(kernel_key_data), is_fallback_(false) {}

  constexpr static int MAX_SIZE = 691;

  bool operator==(const KernelKey& other) const {
    return this->equals(other);
  }

  bool operator!=(const KernelKey& other) const {
    return !this->equals(other);
  }

  bool equals(const KernelKey& other) const {
    if (is_fallback_ != other.is_fallback_) {
      return false;
    }
    if (is_fallback_) {
      return true;
    }
    return strncmp(kernel_key_data_, other.kernel_key_data_, MAX_SIZE) == 0;
  }

  bool is_fallback() const {
    return is_fallback_;
  }

  const char* data() const {
    return kernel_key_data_;
  }

#if defined(ET_OP_REGISTRY_DEBUG)
  friend std::ostream& operator<<(std::ostream& os, const KernelKey& key) {
    os << key.kernel_key_data_ << std::endl;
    return os;
  }
#endif

 private:
  const char* kernel_key_data_ = nullptr;
  bool is_fallback_;
};

/**
 * Struct that bundles a kernel key, a function and an op name together. An
 * `Operator` may have more than one `Kernel` (maximum kMaxNumOfKernelPerOp) and
 * they should have the same op name and different kernel key. A "fallback"
 * kernel may or may not live in an `Operator`.
 */
struct Kernel {
  const char* name_;
  // String representation of kernel key, with the same format as
  // KernelKey.to_string_representation()
  // Data is not owned by the Kernel struct.
  KernelKey kernel_key_;
  OpFunction op_;
  /**
   * We are doing a copy of the string pointer instead of duplicating the string
   * itself, we require the lifetime of the operator name to be at least as long
   * as the operator registry.
   */
  explicit Kernel(const char* name, OpFunction func) : name_(name), op_(func) {}

  explicit Kernel(const char* name, KernelKey key, OpFunction func)
      : name_(name), kernel_key_(key), op_(func) {}

  Kernel() {}
};

namespace internal {
void make_kernel_key_string(Span<const TensorMeta> key, char* buf);
} // namespace internal

/**
 * Checks whether an operator exists with a given name and TensorMeta list. When
 * TensorMeta is empty, it means this op does not have specialized kernels, so
 * it checks whether it has any fallback kernels.
 */
bool registry_has_op_function(
    const char* name,
    Span<const TensorMeta> meta_list = {});

/**
 * Returns the operator with a given name and TensorMeta list, if present.
 */
::executorch::runtime::Result<OpFunction> get_op_function_from_registry(
    const char* name,
    Span<const TensorMeta> meta_list = {});

/**
 * Returns all registered kernels.
 */
Span<const Kernel> get_registered_kernels();

/**
 * Registers the provided kernels.
 *
 * @param[in] kernels Kernel objects to register.
 * @retval Error::Ok always. Panics on error. This function needs to return a
 *     non-void type to run at static initialization time.
 */
ET_NODISCARD Error register_kernels(const Span<const Kernel>);

/**
 * Registers a single kernel.
 *
 * @param[in] kernel Kernel object to register.
 * @retval Error::Ok always. Panics on error. This function needs to return a
 *     non-void type to run at static initialization time.
 */
ET_NODISCARD inline Error register_kernel(const Kernel& kernel) {
  return register_kernels({&kernel, 1});
};

} // namespace runtime
} // namespace executorch

namespace torch {
namespace executor {
// TODO(T197294990): Remove these deprecated aliases once all users have moved
// to the new `::executorch` namespaces.
using ::executorch::runtime::Kernel;
using ::executorch::runtime::KernelKey;
using ::executorch::runtime::KernelRuntimeContext;
using ::executorch::runtime::OpFunction;
using ::executorch::runtime::TensorMeta;
using KernelRuntimeContext = ::executorch::runtime::KernelRuntimeContext;

inline ::executorch::runtime::Error register_kernels(ArrayRef<Kernel> kernels) {
  return ::executorch::runtime::register_kernels(
      {kernels.data(), kernels.size()});
}
inline OpFunction getOpsFn(
    const char* name,
    ArrayRef<TensorMeta> meta_list = {}) {
  auto result = ::executorch::runtime::get_op_function_from_registry(
      name, {meta_list.data(), meta_list.size()});
  ET_CHECK(result.ok()); // get_op_function_from_registry() logs details.
  return *result;
}
inline bool hasOpsFn(const char* name, ArrayRef<TensorMeta> meta_list = {}) {
  return ::executorch::runtime::registry_has_op_function(
      name, {meta_list.data(), meta_list.size()});
}
inline ArrayRef<Kernel> get_kernels() {
  Span<const Kernel> kernels = ::executorch::runtime::get_registered_kernels();
  return ArrayRef<Kernel>(kernels.data(), kernels.size());
}
} // namespace executor
} // namespace torch
