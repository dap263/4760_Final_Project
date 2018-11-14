import csv

path = r"G:\My Drive\Classes\4760\Final Project\hygdata_v3.csv"
stars_data = []
with open(path) as csvfile:
    reader = csv.DictReader(csvfile)
    for row in reader:
        stars_data.append(row)

print(stars_data[1]("id"))