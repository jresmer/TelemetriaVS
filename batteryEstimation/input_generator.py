"""
This scripts generates labeled data that will be used for testing the regression models before the actual data is collected
"""
import sys
import numpy
from os import path
from random import randint


def generate_quadratic_data() -> list | tuple:
    # get random values for weights
    w = randint(0, 10000) / randint(0, 10000), randint(0, 10000) / randint(0, 10000)
    labeled_data = []
    # get target values for given input x
    for i in range(1000):

        x = (i - 500) / 200
        labeled_data.append((x, w[0] + w[1] * numpy.pow(x, 2)))


    # write labaled data to file
    with open(f"quadratic_weights={w}.txt", "w") as f:

        data_string = ""
        for x, y in labeled_data:
            data_string += f"{x}, {y}\n"
        
        f.write(data_string)

def generate_exponetial_data() -> list | tuple:
    # get random values for weights
    w = randint(0, 10000) / randint(0, 10000), randint(0, 10000) / randint(0, 10000)
    labeled_data = []
    # get target values for given input x
    for i in range(1000):

        x = (i - 500) / 200
        labeled_data.append((x, w[0] + w[1] * numpy.exp(w[2] * x)))

    with open(f"quadratic_weights={w}.txt", "w") as f:
        # write labaled data to file
        data_string = ""
        for x, y in labeled_data:
            data_string += f"{x}, {y}\n"

        f.write(data_string)

def main():

    if __name__ == "__main__":

        # standard execution when there's no parameters given
        if len(sys.argv) == 1:
            print("Generating data for both quadratic and exponetial models")
            # generating quadratic data w1 + w2.x^2
            generate_quadratic_data()

            # generating exponential data w1 + w2.e^(x.w3)
            generate_quadratic_data()
        
        elif len(sys.argv) == 2:
            # matching generation algorithm
            match int(sys.argv[1]):
                case 1:
                    # generating quadratic data w1 + w2.x^2
                    print("Generating data for quadratic model")
                    generate_quadratic_data()
                case 2:
                    # generating exponential data w1 + w2.e^(x.w3)
                    print("Generating data for exponetial model")
                    generate_quadratic_data()
        else:
            sys.exit("Usage: python3 input_generator.py or python3 input_generator.py [algorithm]")


main()