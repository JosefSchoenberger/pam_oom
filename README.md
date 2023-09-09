# pam_oom

Adjust the user's OOM score on session creation.

### Building

Make sure you have the development package for libpam installed. The package is called `libpam0g-dev` under Debian. Then run:
```shell
make
```

### Install

Build first, then install with root privileges:
```shell
make
sudo make install
```

Finally, add a corresponding line to PAM:
```shell
echo 'session optional pam_oom.so absolute 100' | sudo tee -a /etc/pam.d/common-session
```
Adjust the parameters to your liking.

### Parameters

`pam_oom` requires exactly two parameters.
```text
Usage: pam_oom.so [absolute|relative] <score-adj>
```
The first parameter needs to be either `absolute` or `relative`.
If it is `absolute`, the score adjustment of the session is set to `<score-adj>`.
If it is `relative`, `<score-adj>` is added to the score adjustment.
`<score-adj>` is a number between -1000 and 1000 (inclusive).

Note that for `relative`, the final score adjustment is clamped in the range `[-999, 1000]`.
The adjustment value -1000 is treated special, as it prevents the session from ever being terminated by the operating system.
It is given if and only if the previous score was -1000 and `<score-adj>` &leq; 0 or if the previous score was &leq; 0 and `<score-adj>` is -1000.
