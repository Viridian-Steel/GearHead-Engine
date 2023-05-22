import os
import subprocess
import platform
import pathlib

from SetupPython import PythonConfiguration as PythonRequirements

# Make sure everything we need for the setup is installed
PythonRequirements.Validate()

from SetupCmake import CmakeConfiguration as CmakeRequirements
from SetupVulkan import VulkanConfiguration as VulkanRequirements
from SetupPremake import PremakeConfiguration as PremakeRequirements
os.chdir('../') # Change from devtools/scripts directory to root

cmakeInstalled = CmakeRequirements.Validate()
VulkanRequirements.Validate()

premakeInstalled = PremakeRequirements.Validate()

print("\nUpdating submodules...")
subprocess.call(["git", "submodule", "update", "--init", "--recursive"])

vkPath = VulkanRequirements.GetVulkanSDK()

currentPath = str(pathlib.Path().resolve()).replace('\\', '/')



if (cmakeInstalled):
    #                               The batchfile                           Source                                            Output
    #1. GLFW
    subprocess.call([os.path.abspath("./scripts/Compile.bat"), (currentPath + "/GearHead-Engine/vendor/GLFW"), currentPath + "/GearHead-Engine/Libs/GLFW"])
    #2. Spdlog
    subprocess.call([os.path.abspath("./scripts/Compile.bat"), (currentPath + "/GearHead-Engine/vendor/spdlog"), currentPath + "/GearHead-Engine/Libs/spdlog"])


else:
    print("GearHead requires Cmake to generate Dependent Libraries.")