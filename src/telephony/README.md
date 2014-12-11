## Introduction
This is the implementation of the W3C Telephony API for the Crosswalk runtime,
as an external Crosswalk extension.

## Specification
The API specification is located at http://telephony.sysapps.org

## Back-end
The back-end on Tizen IVI is oFono. Check documentation at
http://git.kernel.org/cgit/network/ofono/ofono.git/tree/doc/
and source code at
http://git.kernel.org/cgit/network/ofono/ofono.git/.

## Testing
On the test page, first add service and call listeners.
Then get services, and get default service.
Then make a call, check active call, list calls, hold/resume/disconnect etc.

Hints for testing on a device supporting either built-in telephony modems
and/or Bluetooth HFP.

Install packages needed for testing:
zypper in bluez-test ofono-test tizen-extensions-crosswalk-examples

Also, install helpers:
zypper in glibc-devel glibc-devel-utils linux-glibc-devel gdb findutils-locate

Unblock the rfkill on Bluetooth:
rfkill list
Identify the number for Bluetooth and unblock
rfkill unblock 0

Enable xwalk verbose logging, as root:
vi /usr/lib/systemd/user/xwalk.service
Modify xwalk.service to include "--enable-logging --v=1"

Enable core dumps, as root:
chsmack -e System /lib/systemd/systemd-coredump

If want to coredump into files instead of systemd-coredumpctl, as root:
sysctl kernel.core_pattern=core
sysctl kernel.core_uses_pid=1

Set limits for core, as app:
ulimit -c unlimited

Set up environment (and proxy) as app:
export XDG_RUNTIME_DIR=/run/user/`id -u`

systemctl --user daemon-reload

Pair and connect a phone, as app:
bluetoothctl
 list
 agent on
 discoverable on
 pairable on
 scan on
 pair <device>
 connect <device>
 quit

To stop crosswalk, as app:
systemctl --user stop xwalk

To launch the test page:
xwalk-launcher file:///home/app/telephony_test.html

To debug, attach gdb to the xwalk-launcher process (it becomes the extension
process for the app).
