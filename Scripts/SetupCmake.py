	import os
import subprocess
from packaging import version
from pathlib import Path

import Utils

class CmakeConfiguration:
    cmakeVersion = "3.26.3"
    cmakeZipUrls = f"https://github.com/Kitware/CMake/releases/download/v{cmakeVersion}/cmake-{cmakeVersion}-windows-x86_64.zip"
    cmakeDirectory = "./Vendor/Cmake"

    cmakePath = ""

    externalCmake = True

    @classmethod
    def UseExternal(cls) -> bool:
        return cls.externalCmake
    
    @classmethod
    def GetPath(cls) -> str:
        return cls.cmakePath

    @classmethod
    def Validate(cls):
        if (not cls.CheckIfCmakeInstalled()):
            print("Cmake is not installed.")
            return False

        print(f"Correct Cmake located at {os.path.abspath(cls.cmakeDirectory)}")
        return True

    @classmethod
    def CheckIfCmakeInstalled(cls)-> bool: 

        #check if pre-existing version exists

        cmakeExe = Path(f"{os.path.abspath(cls.cmakeDirectory)}/cmake-{cls.cmakeVersion}-windows-x86_64/bin/cmake.exe")
        if(cmakeExe.exists()):
           cls.externalCmake = False
           cls.cmakePath = str(cmakeExe)
           return True

        try:
            out = subprocess.check_output(['cmake', '--version']).decode('utf-8')
        except FileNotFoundError:
            print("file not found")
            return cls.InstallCmake()
        except subprocess.CalledProcessError: #unlikely but you never know
            print("call not work")
            return cls.InstallCmake()
        
        line = out.splitlines()[0]
        cmakeversion = line.split()[2]

        if version.parse(cmakeversion) < version.parse("3.10"):
            print("Version of Cmake incompatible.\n")
            return cls.InstallCmake()



        return True

    @classmethod
    def InstallCmake(cls):
        permissionGranted = False
        while not permissionGranted:
            reply = str(input("Cmake not found. Would you like to download Cmake {0:s}? [Y/N]: ".format(cls.cmakeVersion))).lower().strip()[:1]
            if reply == 'n':
                return False
            permissionGranted = (reply == 'y')

        cmakePath = f"{cls.cmakeDirectory}/cmake-{cls.cmakeVersion}-windows-x86_64.zip"
        print("Downloading {0:s} to {1:s}".format(cls.cmakeZipUrls, cmakePath))
        Utils.DownloadFile(cls.cmakeZipUrls, cmakePath)
        print("Extracting", cmakePath)
        Utils.UnzipFile(cmakePath, deleteZipFile=True)
        print(f"Cmake {cls.cmakeVersion} has been downloaded to '{cls.cmakeDirectory}'")

        return True
