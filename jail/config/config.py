import toml

#deal with config
def parse_config(config_file):
    with open(config_file) as f:
        config = toml.load(f)
    return config
