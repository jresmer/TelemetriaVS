import csv
import os
import sys
import serial


class CSVManager:

    def __init__(self, filename: str="coef.csv"):

        self.__filename = filename
        self.__field_names = ["Tensão"]

    # writes csv file header
    def write_header(self) -> bool:

        # if file does not exist
        if not os.path.exists(self.__filename):

            # create a new file and write
            with open(self.__filename, "wt") as csv_file:

                writer = csv.DictWriter(csv_file, fieldnames=self.__field_names)
                writer.writeheader()

            return True

        # if file exists and is empty
        elif os.stat(self.__filename).st_size == 0:

            # write header
            with open(self.__filename, "a") as csv_file:

                writer = csv.DictWriter(csv_file, fieldnames=self.__field_names)
                writer.writeheader()

            return True
        
        return False

    # writes new row
    def write_row(self, 
                  T: str) -> bool:
        
        # if file exists and is not empty (has at least a header written in it)
        if os.path.exists(self.__filename) and os.stat(self.__filename).st_size != 0:
        
            # associates contents to columns
            row_content = {
                "Tensão": T
            }

            # writes row
            with open(self.__filename, "a", newline="") as csv_file:

                writer = csv.DictWriter(csv_file, fieldnames=self.__field_names)
                writer.writerow(row_content)

def main():

  if len(sys.argv) == 1:
    path = "coef.csv"
  elif len(sys.argv) == 2:
    path = sys.argv[1]
  
  print("Por padrão o arquivo gerado será coef.csv")
  if os.path.exists(path) and os.stat(path).st_size != 0:
    print("O arquivo já existe e não é vazio")
    print("Encerrando o programa")
    sys.exit("Uso python3 main.py <nome do aquivo a ser criado>")

  manager = CSVManager(path)
  manager.write_header()

  ser = serial.Serial(port="", # porta serial conectada com o microcontrolador
		    baudrate=9600, # ajuste a frequencia do controlador
		    )

  while True:

    for line in ser.read():
      manager.write_row(line)
