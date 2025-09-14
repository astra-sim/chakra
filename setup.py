from setuptools.command.build import build as _build

from setuptools import setup


class custom_build(_build):
    """
    Custom build class to include gRPC build commands.

    This class modifies the default build process to include additional sub-commands
    necessary for building gRPC components.

    Attributes
        sub_commands (list): List of sub-commands to be executed during the build process.
    """

    sub_commands = [("build_grpc", None)] + _build.sub_commands


setup(cmdclass={"build": custom_build})
