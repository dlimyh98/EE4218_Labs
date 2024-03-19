NUM_ROWS = 64
NUM_COLUMNS = 8

def convert_vivado(test_cases):
    test_input_file = open('test_input.mem', 'w')
    test_result_file = open('test_result_expected.mem', 'w')

    for test_case in test_cases:
        test_input_file.write("//input vector\n")

        # Test cases are in the first two elements of the sub-list only
        for test_case_file in test_case[:2]:
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

        # Expected is always the 3rd element of the sub-list
        read_file = open(test_case[2])
        read_lines = read_file.readlines()

        for read_line in read_lines:
            read_line = read_line.rstrip()
            hex_value = f"{(int)(read_line):#0{4}x}"
            test_result_file.write(hex_value + "\n")

        test_result_file.close()



    # test_input_file contains values for ALL test case scenarios
    test_input_file.close()


def convert_vitis(test_cases):
    test_input_file = open('converted_test_input.txt', 'w')
    test_result_file = open('converted_test_result.txt', 'w')

    for test_case in test_cases:

        # Test cases are in the first two elements of the sub-list only
        for test_case_file in test_case[:2]:
            read_file = open(test_case_file, 'r')
            read_lines = read_file.readlines()
            B_counter = 0

            for read_line in read_lines:
                read_line = read_line.rstrip()
                decimal_values = read_line.split(",")
                A_counter = 0

                for decimal_value in decimal_values:
                    # https://stackoverflow.com/questions/12638408/decorating-hex-function-to-pad-zeros
                    hex_value = f"{(int)(decimal_value):#0{4}x}"

                    if (test_case_file == test_case[0]):
                        if (A_counter != NUM_COLUMNS-1):
                            test_input_file.write(hex_value + ",")
                            A_counter+=1
                        else:
                            test_input_file.write(hex_value + ",\n")
                    else:
                        if (B_counter != NUM_COLUMNS-1):
                            test_input_file.write(hex_value + ",")
                            B_counter+=1
                        else:
                            test_input_file.write(hex_value)
                        
            read_file.close()

        # Expected is always the 3rd element of the sub-list
        read_file = open(test_case[2])
        read_lines = read_file.readlines()
        counter = 0

        for read_line in read_lines:
            read_line = read_line.rstrip()
            hex_value = f"{(int)(read_line):#0{4}x}"

            if (counter != NUM_COLUMNS-1):
                test_result_file.write(hex_value + ",")
            else:
                test_result_file.write(hex_value + ",\n")
                counter-=8

            counter += 1
        test_result_file.close()

    # test_input_file contains values for ALL test case scenarios
    test_input_file.close()


if __name__ == "__main__":
    test_cases = [["A_rajesh.txt", "B_rajesh.txt", "expected_rajesh.txt"]]

    #convert_vivado(test_cases)
    convert_vitis(test_cases)