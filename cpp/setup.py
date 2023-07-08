from distutils.core import setup, Extension


def main():
  setup(
    name="MotorDriver",
    version="1.0.0",
    description="MotorDriver module in python",
    author="Gabs",
    author_email="gabs@imshi.co.uk",
    ext_modules=[Extension("MotorDriver", ["MotorDriver.c"], libraries=['bcm2835'])]
  )

if (__name__ == "__main__"):
  main()