import re

filepath1 = "./parser/2024-10-27-run.TXT"

with open(filepath1, "r") as f:
    content = f.readlines()

with open("./parser/outfile.txt", "w") as f:
    for line in content:
        match = re.search(r"(\d+) ms,  Energy Used: ([\d.]+) Cumulative Energy:([\d.]+)", line)
        if match:
            ms = match.group(1)
            energyUsed = match.group(2)
            cumulativeEnergy = match.group(3)
            f.write(f"\n{ms},,,{energyUsed},{cumulativeEnergy},") # 2 empty slots for current and voltage
            
        match = re.search(r"MPGe:([\d.]+)Lat/Long: ([\d.-]+), ([\d.-]+) Trip:([\d.]+) Distance:([\d.]+) Date: (\d{1,2}/\d{1,2}/\d{4})  Time: (\d{2}:\d{2}:\d{2}\.\d) Course \(degrees\): ([\d.-]+) Speed\(mph\): ([\d.]+)", line)
        if match:
            mpge = match.group(1)
            lat = match.group(2)
            long = match.group(3)
            trip = match.group(4)
            distance = match.group(5)
            date = match.group(6)
            time = match.group(7)
            course = match.group(8)
            speed = match.group(9)
            f.write(f"{mpge},{lat},{long},{trip},{distance},{date},{time},{course},{speed}")

        match = re.search(r"(\d+),([\d.]+),([\d.]+)", line)
        if match:
            ms = match.group(1)
            current = match.group(2)
            voltage = match.group(3)
            f.write(f"\n{ms},{current},{voltage}")