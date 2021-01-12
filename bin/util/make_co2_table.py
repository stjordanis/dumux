""" Generate tables for CO2 fluid properties.

The tables are generated using the NIST (National Institute of Standards
and Techlology) Standard Reference Database Number 69
(https://doi.org/10.18434/T4D303).

Copyright for NIST Standard Reference Data is governed by the Standard
Reference Data Act (https://www.nist.gov/srd/public-law). Make sure to use
this script in accordance with the Standard Reference Data Act.
the SRDA.

The values are calculated using the equation of Span and Wagner """

import requests
import pandas as pd
from io import StringIO
from string import Template

# TODO: Shall the min and max values be included in the table?
# TODO: Is NUM_STEPS the number of samples or the really number of steps?
# TODO: Check why the values differ from the reference values!
# TODO: Shall I better use a class

NUM_TEMP_STEPS = 50
MIN_TEMP = 2.900000000000000e+02  # K
MAX_TEMP = 3.400000000000000e+02  # K
NUM_PRESS_STEPS = 495
MIN_PRESS = 1.000000000000000e+05  # Pa
MAX_PRESS = 1.000000000000000e+08  # Pa


def formate_values(values):
    text = '    {\n       '
    for i, value in enumerate(values):
        if i == len(values)-1:
            text += '     ' + format(value, '.15e') + '\n    },\n'
            return text
        text += '     ' + format(value, '.15e') + ','
        if (i + 1) % 5 == 0:
            text += ' \n       '


delta_temperature = (MAX_TEMP - MIN_TEMP) / (NUM_TEMP_STEPS - 1)
delta_pressure = (MAX_PRESS - MIN_PRESS) / (NUM_PRESS_STEPS - 1)

density_vals = ''
enthalpy_vals = ''

# get the data
for i in range(NUM_TEMP_STEPS):
    temperature = MIN_TEMP + i * delta_temperature
    response = requests.get('https://webbook.nist.gov/cgi/fluid.cgi?Action=Data&Wide=on&ID=C7732185&Type=IsoTherm&Digits=5&PLow='
                            + str(MIN_PRESS) + '&PHigh=' + str(MAX_PRESS) + '&PInc=' + str(delta_pressure) + '&T=' + str(temperature)
                            + '&RefState=DEF&TUnit=K&PUnit=Pa&DUnit=mol%2Fl&HUnit=kJ%2Fmol&WUnit=m%2Fs&VisUnit=uPa*s&STUnit=N%2Fm')
    response.encoding = 'utf-8'
    text = response.text
    raw_data = StringIO(text)
    df = pd.read_csv(raw_data, sep='\t')

# formate the data
    temp = df["Density (mol/l)"]
    density_vals += formate_values(df["Density (mol/l)"])
    enthalpy_vals += formate_values(df["Enthalpy (kJ/mol)"])

density_vals = density_vals[:-2]
enthalpy_vals = enthalpy_vals[:-2]

# write the table by filling the gaps in the template
f = open("co2values_template.inc", 'r')
template = Template(f.read())
replacements = {"NUM_TEMP_STEPS": format(NUM_TEMP_STEPS),
                "MIN_TEMP": format(MIN_TEMP, '.15e'),
                "MAX_TEMP": format(MAX_TEMP, '.15e'),
                "NUM_PRESS_STEPS": format(NUM_PRESS_STEPS),
                "MIN_PRESS": format(MIN_PRESS, '.15e'),
                "MAX_PRESS": format(MAX_PRESS, '.15e'),
                "DENSITY_VALS": density_vals,
                "ENTHALPY_VALS": enthalpy_vals}
text_output = template.substitute(replacements)

f = open("co2values.inc", 'w')
f.write(text_output)
f.close()
