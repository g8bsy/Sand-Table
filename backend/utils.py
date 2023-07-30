import re

def pi_version():
    """Detect the version of the Raspberry Pi.  Returns either 1, 2 or
    None depending on if it's a Raspberry Pi 1 (model A, B, A+, B+),
    Raspberry Pi 2 (model B+), or not a Raspberry Pi.
    """
    # Check /proc/cpuinfo for the Hardware field value.
    # 2708 is pi 1
    # 2709 is pi 2
    # 2835 is pi 3 on 4.9.x kernel
    # Anything else is not a pi.
    try:
        with open('/proc/cpuinfo', 'r') as infile:
            cpuinfo = infile.read()
        # Match a line like 'Hardware   : BCM2709'
        match = re.search('^Hardware\s+:\s+(\w+)$', cpuinfo,
                        flags=re.MULTILINE | re.IGNORECASE)
        if not match:
            # Couldn't find the hardware, assume it isn't a pi.
            return None
        if match.group(1) == 'BCM2708':
            # Pi 1
            return 1
        elif match.group(1) == 'BCM2709':
            # Pi 2
            return 2
        elif match.group(1) == 'BCM2835':
            # Pi 3 / Pi on 4.9.x kernel
            return 3
        else:
            # Something else, not a pi.
            return None
    except Exception as e:
        print(e)