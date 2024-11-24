import mpemba
import sys
import os

if __name__ == '__main__':
    file_list = sys.argv
    file_list.pop(0)  # Remove the code from the argument list
    for file in file_list:
        if not os.path.isfile(file):
            raise Exception("One of the provided case files does not exist.")

    for file in file_list:
        try:
            results = mpemba.Results(file)
            results.save()
            results.print()

        except Exception as e:
            print(e)
            print("An error occurred while processing the experiment file")
            print(file)
            sys.exit(1)
