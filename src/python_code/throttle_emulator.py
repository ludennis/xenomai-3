#!/usr/bin/python3

import time
import readchar
import sys

if __name__ == '__main__':
    throttle = 0
    print('Press w to increase throttle, s to decrease throttle, q to quit')

    while True:
        print('Throttle = {}'.format(throttle))
        keyboardInput = readchar.readkey()
        print("keyboardInput: {}".format(keyboardInput))
        if keyboardInput == 'w':
            print("increasing throttle")
            if throttle < 100:
                throttle += 1;
        elif keyboardInput == 'q':
            sys.exit()
        else:
            if throttle > 0:
                throttle -= 1;
