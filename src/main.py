from .utils.constants import APP_VERSION as VERSION

from .controllers.main_controller import main

if __name__ == "__main__":
    import sys
    sys.exit(main(VERSION))
