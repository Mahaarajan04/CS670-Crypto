import os

def load_env(filename=".env"):
    env = {}
    with open(filename) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            if "=" in line:
                k, v = line.split("=", 1)
                env[k.strip()] = v.strip()
    return env

def generate_dockerfile(p):
    header = """FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \\
    g++ cmake make python3 \\
    libboost-all-dev \\
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

"""
    # Build commands in a single RUN block with continuations
    cmds = "RUN g++ -std=c++17 gen_queries.cpp -o gen_queries \\\n" \
           " && g++ -std=c++17 dealer.cpp -o dealer \\\n" \
           " && g++ -std=c++17 checker.cpp -o check"

    for i in range(p):
        cmds += f" \\\n && g++ -std=c++17 -DROLE_p{i} pB.cpp -o p{i}"

    return header + cmds + "\n"

def generate_compose(p):
    header = """version: "3.9"

services:
  gen_queries:
    build: .
    container_name: gen_queries
    command: ./gen_queries
    env_file:
      - .env
    volumes:
      - ./data:/app/data
    networks:
      - mpc_net

  dealer:
    build: .
    container_name: dealer
    command: ./dealer
    env_file:
      - .env
    volumes:
      - ./data:/app/data
    depends_on:
      gen_queries:
        condition: service_completed_successfully
    networks:
      - mpc_net
"""
    parties = ""
    for i in range(p):
        parties += f"""
  p{i}:
    build: .
    container_name: p{i}
    command: ./p{i}
    env_file:
      - .env
    volumes:
      - ./data:/app/data
    depends_on:
      dealer:
        condition: service_started
      gen_queries:
        condition: service_completed_successfully"""
        if i > 0:
            parties += f"""
      p{i-1}:
        condition: service_started"""
        parties += """
    networks:
      - mpc_net
"""
    footer = """
networks:
  mpc_net:
    driver: bridge
"""
    return header + parties + footer


if __name__ == "__main__":
    env = load_env()
    if "P" not in env:
        print("Error: P (number of parties) not found in .env")
        exit(1)

    p = int(env["P"])

    # Write Dockerfile
    with open("Dockerfile", "w") as f:
        f.write(generate_dockerfile(p))
    print(f"Generated Dockerfile for {p} parties.")

    # Write docker-compose.yml
    with open("docker-compose.yml", "w") as f:
        f.write(generate_compose(p))
    print(f"Generated docker-compose.yml for {p} parties.")
