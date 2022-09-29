#!/usr/bin/env python3

import sys
from warnings import filters
import yaml

with open("config_settings.yaml") as file:
    config_settings = yaml.safe_load(file)

with open("variables.yaml") as file:
    variables = yaml.safe_load(file)

with open("lua.yaml") as file:
    lua = yaml.safe_load(file)

data = {
    "config_settings": config_settings,
    "variables": variables,
    "lua": lua,
}

from jinja2 import Environment, FileSystemLoader, select_autoescape


def reverse_format(param_list, format_string):
    return format_string.format(param_list)


env = Environment(
    loader=FileSystemLoader("."),
    autoescape=select_autoescape(),
)
env.filters["reverse_format"] = reverse_format

template = env.get_template(sys.argv[1])
print(template.render(data))
