NUM_ROWS = 64
NUM_COLUMNS = 8

def convert_vivado(test_cases):
    test_input_file = open('test_input.mem', 'w')

    for test_case in test_cases:
        test_input_file.write("//input vector\n")

        # Test cases are in the first three elements of the sub-list only
        for test_case_file in test_case[:3]:
            read_file = open(test_case_file, 'r')
            read_lines = read_file.readlines()

            for read_line in read_lines:
                read_line = read_line.rstrip()
                decimal_values = read_line.split(",")

                for decimal_value in decimal_values:
                    # https://stackoverflow.com/questions/12638408/decorating-hex-function-to-pad-zeros
                    hex_value = f"{(int)(decimal_value):#0{4}x}"
                    test_input_file.write(hex_value + "\n")

            read_file.close()

    test_input_file.close()


if __name__ == "__main__":
    test_cases = [["X.csv", "w_hid.csv", "w_out.csv"]]

    convert_vivado(test_cases)
    #convert_vitis(test_cases)