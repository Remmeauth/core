from testnet_bootstrap import producers
from start_bootnode import start_node

producers_quantity = 3
nodes_quantity = 0


def start_nodes():
    for i in range(1, nodes_quantity+1):
        start_node(i, producers['producers'][i-1])


def start_producers():
    start_node(1, producers['producers'][0:producers_quantity])


if __name__ == '__main__':
    if nodes_quantity:
        start_nodes()
    if producers_quantity:
        start_producers()
