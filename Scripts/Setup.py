import os
import subprocess
import platform
import pathlib
import shutil as sh
from command_runner.elevate import elevate

from SetupPython import PythonConfiguration as PythonRequirements

# Make sure everything we need for the setup is installed
PythonRequirements.Validate()

from SetupCmake import CmakeConfiguration as CmakeRequirements
from SetupVulkan import VulkanConfiguration as VulkanRequirements
from SetupPremake import PremakeConfiguration as PremakeRequirements
os.chdir('../') # Change from devtools/scripts directory to root

def CompileLibs():
    #Compile necesary Libraries and place them in the correct location
    #                               The batchfile                           Source                                            Output
    #1. GLFW
    subprocess.call([os.path.abspath("./scripts/Compile.bat"), (currentPath + "/GearHead-Engine/vendor/GLFW"), currentPath + "/GearHead-Engine/Libs/GLFW"])
    #2. Spdlog
    subprocess.call([os.path.abspath("./scripts/Compile.bat"), (currentPath + "/GearHead-Engine/vendor/spdlog"), currentPath + "/GearHead-Engine/Libs/spdlog"])

def  InstallIncludes():
    try:
        if(os.path.isdir(os.path.abspath("./GearHead-Engine/Include")) == False):
            os.mkdir(os.path.abspath("./GearHead-Engine/Include"))
        sh.copytree(os.path.abspath("./GearHead-Engine/vendor/GLM/glm"),os.path.abspath("./GearHead-Engine/Include/glm"))
        return True
    except PermissionError:
        return PermissionError
    except FileExistsError:
        return FileExistsError
    

cmakeInstalled = CmakeRequirements.Validate()
VulkanRequirements.Validate()

premakeInstalled = PremakeRequirements.Validate()

print("\nUpdating submodules...")
subprocess.call(["git", "submodule", "update", "--init", "--recursive"])

vkPath = VulkanRequirements.GetVulkanSDK()

currentPath = str(pathlib.Path().resolve()).replace('\\', '/')

if (cmakeInstalled and premakeInstalled):
    CompileLibs()

    e = InstallIncludes()

    if(e == PermissionError):
        print("Requires Elevated commands")
        elevate(InstallIncludes)
    elif(e == FileExistsError):
        print("Include Files Already In Place")
    
    subprocess.call([currentPath + "/vendor/premake/bin/premake5.exe", "vs2022"])

else:
    print("GearHead requires Cmake and Preamek to generate Project.")