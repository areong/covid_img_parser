import datetime
import unittest
import date_formatter

class TestDateFormatter(unittest.TestCase):
    def test_increment_datetime_day(self):
        # Pairs of input datatime and expected output datetime.
        datetime_pairs = [
            (datetime.datetime(2022, 7, 1), datetime.datetime(2022, 7, 2)),
            (datetime.datetime(2022, 7, 31), datetime.datetime(2022, 8, 1)),
            (datetime.datetime(2022, 8, 15), datetime.datetime(2022, 8, 16)),
            (datetime.datetime(2022, 8, 31), datetime.datetime(2022, 9, 1)),
            (datetime.datetime(2022, 9, 1), datetime.datetime(2022, 9, 2)),
            (datetime.datetime(2022, 9, 30), datetime.datetime(2022, 10, 1)),
            (datetime.datetime(2022, 12, 31), datetime.datetime(2023, 1, 1)),
            ]
        for input_datetime, expected_output_datetime in datetime_pairs:
            self.assertEqual(date_formatter.increment_datetime_day(input_datetime),
                expected_output_datetime)

    def test_increment_datetime_month(self):
        # Pairs of input datatime and expected output datetime.
        datetime_pairs = [
            (datetime.datetime(2022, 1, 1), datetime.datetime(2022, 2, 1)),
            (datetime.datetime(2022, 1, 15), datetime.datetime(2022, 2, 15)),
            (datetime.datetime(2022, 1, 31), datetime.datetime(2022, 2, 28)),
            (datetime.datetime(2022, 7, 1), datetime.datetime(2022, 8, 1)),
            (datetime.datetime(2022, 7, 15), datetime.datetime(2022, 8, 15)),
            (datetime.datetime(2022, 7, 31), datetime.datetime(2022, 8, 31)),
            (datetime.datetime(2022, 8, 1), datetime.datetime(2022, 9, 1)),
            (datetime.datetime(2022, 8, 15), datetime.datetime(2022, 9, 15)),
            (datetime.datetime(2022, 8, 31), datetime.datetime(2022, 9, 30)),
            (datetime.datetime(2022, 9, 1), datetime.datetime(2022, 10, 1)),
            (datetime.datetime(2022, 9, 15), datetime.datetime(2022, 10, 15)),
            (datetime.datetime(2022, 9, 30), datetime.datetime(2022, 10, 30)),
            (datetime.datetime(2022, 12, 1), datetime.datetime(2023, 1, 1)),
            (datetime.datetime(2022, 12, 15), datetime.datetime(2023, 1, 15)),
            (datetime.datetime(2022, 12, 31), datetime.datetime(2023, 1, 31)),
            ]
        for input_datetime, expected_output_datetime in datetime_pairs:
            self.assertEqual(date_formatter.increment_datetime_month(input_datetime),
                expected_output_datetime)

if __name__ == '__main__':
    unittest.main()
