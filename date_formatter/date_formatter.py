import datetime
import sys

def increment_datetime_day(input_datetime):
    '''
    Increment datetime by one day.
    '''
    return input_datetime + datetime.timedelta(days=1)

def increment_datetime_month(input_datetime):
    '''
    Increment datetime by one month while keeping the day value unchanged.
    '''
    input_day = input_datetime.day

    # Increment month by adding a time delta.
    if input_day <= 16:
        output_datetime = input_datetime + datetime.timedelta(days=32)
    else:
        output_datetime = input_datetime + datetime.timedelta(days=16)

    # Assign the input day to the output datetime.
    # The input day can be larger than the maximum day of the output month.
    # Find the largest output valid day in the output month.
    # The output valid day should be not larger than the input day.
    output_day = input_day
    while output_day > 0:
        try:
            output_datetime = datetime.datetime(output_datetime.year,
                output_datetime.month, output_day)
            break
        except ValueError:
            output_day -= 1

    return output_datetime

def expand_date(date_texts):
    '''
    For each input date text, convert it to a date duration, and then output
    all days in the durations.

    For example, input ['12/30-1/2', '6/7', '7/8-10'] will be converted to
    [[2021/12/30, 2021/12/31, 2022/1/1, 2022/1/2], [2022/6/7], [2022/7/8,
    2022/7/9, 2022/7/10]], where each element is a datetime instance.
    Assume the current year in the example is 2022.
    '''
    output_datetimes = []
    this_year = datetime.datetime.today().year
    one_day_delta = datetime.timedelta(days=1)

    for date_text in date_texts:
        # Parse the text.
        hyphen_position = date_text.find('-')
        second_slash_position = date_text.find('/', hyphen_position)
        if hyphen_position < 0:
            # Date text format: '%m/%d'.
            # Convert to a datetime.
            datetime_0 = datetime.datetime.strptime(str(this_year) + '/' +
                date_text, '%Y/%m/%d')

            # Let date 1 the same with date 0, such that the duration is zero.
            datetime_1 = datetime_0
        elif second_slash_position > 0:
            # Date text format: '%m/%d-%m/%d'
            date_text_0 = date_text[:hyphen_position]
            date_text_1 = date_text[hyphen_position + 1:]

            # Convert to datetimes.
            datetime_0 = datetime.datetime.strptime(str(this_year) + '/' +
                date_text_0, '%Y/%m/%d')
            datetime_1 = datetime.datetime.strptime(str(this_year) + '/' +
                date_text_1, '%Y/%m/%d')

            # Cross-year date:
            # If month 1 is smaller than month 0, assign year 0 the last year.
            # Assume the current year is larger than 1, and assume the duration
            # is within one month (so 2/29 is not in concern).
            if datetime_0.month > datetime_1.month:
                datetime_0 = datetime.datetime(this_year - 1, datetime_0.month,
                    datetime_0.day)
        else:
            # Date text format: '%m/%d-%d'
            # Assume both dates are in the same month.
            date_text_0 = date_text[:hyphen_position]
            date_text_1 = date_text[hyphen_position + 1:]

            # Convert the first date text to a datetime.
            datetime_0 = datetime.datetime.strptime(str(this_year) + '/' +
                date_text_0, '%Y/%m/%d')

            # Convert the second date text to a datetime by using the month
            # of the first date.
            datetime_1 = datetime.datetime.strptime(str(this_year) + '/' + 
                str(datetime_0.month) + '/' + date_text_1, '%Y/%m/%d')

        # Populate output with every day in range [datetime_0, datetime_1].
        output_datetimes.append([])
        current_datetime = datetime_0
        while current_datetime <= datetime_1:
            output_datetimes[-1].append(current_datetime)
            current_datetime += one_day_delta

    return output_datetimes

def main():
    if len(sys.argv) < 2:
        print('Usage: python date_formatter.py <comma_separated_dates>')
        print('The comma-separated dates should not contain whitespaces.')
        return -1

    # Split input text at commas.
    comma_separate_date_text = sys.argv[1]
    date_texts = comma_separate_date_text.split(',')

    # Expand date durations into individual days.
    datetimes = expand_date(date_texts)

    # Print results.
    for section in datetimes:
        for datetime_obj in section:
            print(datetime_obj.strftime('%Y/%m/%d'))

    return 0

if __name__ == '__main__':
    main()
