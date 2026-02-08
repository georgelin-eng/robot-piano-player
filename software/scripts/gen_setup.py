import pandas as pd

df = pd.read_csv("IO.csv", delimiter="\t")

f = open("setup.txt", "w")

for i, row in df.iterrows():
    pin_name  = row["Pin"]
    direction = row["Direction"]

    if (direction == "INPUT"):
        output_txt = f"pinMode({pin_name}, INPUT_PULLUP);"   
    else:
        output_txt = f"pinMode({pin_name}, OUTPUT);"   

    output_txt += "\n"
    f.write(output_txt)
