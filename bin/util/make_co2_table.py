""" Generate tables for CO2 fluid properties.

The tables are generated using the NIST (National Institute of Standards
and Techlology) Standard Reference Database Number 69
(https://doi.org/10.18434/T4D303).

Copyright for NIST Standard Reference Data is governed by the Standard
Reference Data Act (https://www.nist.gov/srd/public-law).

The values are calculated using the equation of Span and Wagner """

import urllib
import requests
import numpy as np
from io import StringIO
from string import Template


MIN_TEMP = 290  # K
MAX_TEMP = 340  # K
NUM_TEMP_SAMPLES = 50 # MIN_TEMP ist the first sample point, MAX_TEMP the last.
MIN_PRESS = 1.0e+05  # Pa
MAX_PRESS = 1.0e+08  # Pa
NUM_PRESS_SAMPLES = 495 # MIN_PRESS ist the first sample point, MAX_PRESS the last.

delta_temperature = (MAX_TEMP - MIN_TEMP) / (NUM_TEMP_SAMPLES - 1)
delta_pressure = (MAX_PRESS - MIN_PRESS) / (NUM_PRESS_SAMPLES - 1)

density_str = []
enthalpy_str = []

# get the data
for i in range(NUM_TEMP_SAMPLES):
    temperature = MIN_TEMP + i * delta_temperature
    query = {'Action': 'Data', 'Wide': 'on', 'ID': 'C124389', 'Type': 'IsoTherm',
             'Digits': '12', 'PLow': str(MIN_PRESS), 'PHigh': str(MAX_PRESS),
             'PInc': str(delta_pressure), 'T': str(temperature), 'RefState': 'DEF',
             'TUnit': 'K', 'PUnit': 'Pa', 'DUnit': 'kg/m3', 'HUnit': 'kJ/kg',
             'WUnit': 'm/s', 'VisUnit': 'uPas', 'STUnit': 'N/m'}
    response = requests.get('https://webbook.nist.gov/cgi/fluid.cgi?'
                            + urllib.parse.urlencode(query))
    response.encoding = 'utf-8'
    text = response.text
    phase = np.genfromtxt(StringIO(text), delimiter='\t', dtype=str, usecols=[-1],
                          skip_header=1)
    values = np.genfromtxt(StringIO(text), delimiter='\t', names=True)

# TODO: Decide how to deal with the transition points, which are additionally
#       to the other sampling points provided by NIST. WARNING: The implemented
#       removing of the transition point does not work if there are two
#       transition point within the data range.

# find the phase transition
    values_red = np.copy(values)
    for i in range(1, len(phase)-1):
        if phase[i] != phase[i+1]:
            # remove the phase transition
            density = np.concatenate((values["Density_kgm3"][:i], values["Density_kgm3"][i+2:]))
            enthalpy = np.concatenate((values["Enthalpy_kJkg"][:i], values["Enthalpy_kJkg"][i+2:]))
            # transform unit (kJ/kg -> J/kg)
            enthalpy *= 1000
            # formate the data
            density_str.append('    {'+', '.join([format(x, '.12e') for x in density])+'}')
            enthalpy_str.append('    {'+', '.join([format(x, '.12e') for x in enthalpy])+'}')
            break

density_str = ',\n'.join(density_str)
enthalpy_str = ',\n'.join(enthalpy_str)

# write the table by filling the gaps in the template
f = open("co2values_template.inc", 'r')
template = Template(f.read())
replacements = {"MIN_TEMP": format(MIN_TEMP),
                "MAX_TEMP": format(MAX_TEMP),
                "NUM_TEMP_SAMPLES": format(NUM_TEMP_SAMPLES),
                "MIN_PRESS": format(MIN_PRESS),
                "MAX_PRESS": format(MAX_PRESS),
                "NUM_PRESS_SAMPLES": format(NUM_PRESS_SAMPLES),
                "DENSITY_VALS": density_str,
                "ENTHALPY_VALS": enthalpy_str}
text_output = template.substitute(replacements)

f = open("co2values.inc", 'w')
f.write(text_output)
f.close()
