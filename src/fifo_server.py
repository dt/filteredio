from __future__ import print_function
import os, sys, time

def main():
  print("fifo: " + sys.argv[1])
  os.mkfifo(sys.argv[1])

  whitelist = ['foo', 'foo/bar', 'foo/baz', 'ls.c', 'test.c', 'filter.cpp']

  try:
    while True:
      fifo = open(sys.argv[1], 'w')
      for i in whitelist:
        print(i, file=fifo)
      fifo.flush()
      fifo.close()
      time.sleep(.05)
  finally:
    fifo.close()
    os.remove(sys.argv[1])

if __name__ == "__main__":
  main()
