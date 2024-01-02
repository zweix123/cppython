import pydot

graph = pydot.Dot(graph_type="digraph", bgcolor="#fff3af")
cnt = 0


def recur_tree_viser_help(tree_context: list[int | str]) -> (list, pydot.Node):
    global graph, cnt
    cnt += 1
    node = pydot.Node(name=cnt, label=tree_context[0])
    graph.add_node(node)
    num = tree_context[1]
    tree_context = tree_context[2:]

    for _ in range(num):
        tree_context, son = recur_tree_viser_help(tree_context)
        edge = pydot.Edge(node, son)
        graph.add_edge(edge)
    return tree_context, node


def recur_tree_viser(tree_context: list[int | str], img_name: str):
    recur_tree_viser_help(tree_context)
    graph.write_png(img_name)