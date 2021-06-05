
from platformio.managers.platform import PlatformBase
from platformio.util import get_systype


class P912Platform(PlatformBase):

    @property
    def packages(self):
        packages = PlatformBase.packages.fget(self)
        if (get_systype() == "linux_i686" and
                "toolchain-gcclinux32" in packages):
            del packages['toolchain-gcclinux32']
        return packages
