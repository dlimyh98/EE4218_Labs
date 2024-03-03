def convert(test_cases):
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
                    hex_value = hex(int(decimal_value))
                    test_input_file.write(hex_value + "\n")

            read_file.close()

        # Expected is always the 3rd element of the sub-list
        read_file = open(test_case[2])
        read_lines = read_file.readlines()

        for read_line in read_lines:
            read_line = read_line.rstrip()
            hex_value = hex(int(read_line))
            test_result_file.write(hex_value + "\n")

        test_result_file.close()



    # test_input_file contains values for ALL test case scenarios
    test_input_file.close()

if __name__ == "__main__":
    test_cases = [["A.txt", "B.txt", "expected.txt"]]
    convert(test_cases)