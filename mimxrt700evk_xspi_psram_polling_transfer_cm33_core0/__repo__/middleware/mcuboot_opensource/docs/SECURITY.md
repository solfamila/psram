# Project security policy

The MCUboot team takes security, vulnerabilities, and weaknesses
seriously.

## Reporting security issues

The preferred way to report security issues with MCUboot is via the "Report a
security vulnerability" button on the main [security
page](https://github.com/mcu-tools/mcuboot/security).

You can also directly contact the following maintainers of the project:

- David Brown: davidb@davidb.org or david.brown@linaro.org
- Fabio Utzig: utzig@apache.org

If you wish to send an encrypted email, you may use these PGP keys:

```
    pub   rsa4096 2011-10-14 [SC]
          DAFD760825AE2636AEA9CB19E6BA9F5C5E54DF82
    uid           [ultimate] David Brown <davidb@davidb.org>
    uid           [ultimate] David Brown <david.brown@linaro.org>
    sub   rsa4096 2011-10-14 [E]
```

and

```
    pub   rsa4096 2017-07-28 [SC]
          126087C7E725625BC7E89CC7537097EDFD4A7339
    uid           [ unknown] Fabio Utzig <utzig@apache.org>
    uid           [ unknown] Fabio Utzig <utzig@utzig.org>
    sub   rsa4096 2017-07-28 [E]
```

Please include the word "SECURITY" as well as "MCUboot" in the subject
of any message.

We will make our best effort to respond in a timely manner. Most
vulnerabilities found within published code will undergo an embargo of
90 days to allow time fixes to be developed and deployed.

## Vulnerability advisories

Vulnerability reports and published fixes will be reported as follows:

- Issues will be entered into MCUboot's [security advisory
  system](https://github.com/mcu-tools/mcuboot/security/advisories) on GitHub, with
  the interested parties (including the reporter) added as viewers.

- The release notes will contain a reference to any allocated CVE(s).

- When the embargo is lifted, the security advisory page will be made
  public, and the public CVE database will be updated with all
  relevant information.
