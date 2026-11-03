"""Compile the C implementation and compare it with a Python reference."""

from pathlib import Path
import random
import subprocess
import tempfile


def reference(vertices, edges, source):
    distances = [None] * vertices
    distances[source] = 0
    for _ in range(vertices - 1):
        updated = distances.copy()
        for start, end, weight in edges:
            if distances[start] is not None:
                candidate = distances[start] + weight
                if updated[end] is None or candidate < updated[end]:
                    updated[end] = candidate
        distances = updated
    for start, end, weight in edges:
        if distances[start] is not None and (
            distances[end] is None or distances[start] + weight < distances[end]
        ):
            return None
    return distances


def main():
    directory = Path(__file__).resolve().parent
    source = (directory / "bellman_ford.c").as_posix()
    with tempfile.TemporaryDirectory(prefix="bf-check-", dir=directory) as temporary:
        temporary = Path(temporary)
        harness = temporary / "check.c"
        harness.write_text(
            '#define main sample_main\n#include "' + source + '"\n#undef main\n'
            r'''
int main(void) {
    int vertices, edges, source;
    if (scanf("%d %d %d", &vertices, &edges, &source) != 3) return 2;
    struct Graph *graph = createGraph(vertices, edges);
    if (graph == NULL) return 1;
    for (int i = 0; i < edges; i++) {
        if (scanf("%d %d %d", &graph->edge[i].src,
                  &graph->edge[i].dest, &graph->edge[i].weight) != 3) {
            free(graph->edge);
            free(graph);
            return 2;
        }
    }
    bool success = BellmanFord(graph, source);
    free(graph->edge);
    free(graph);
    return success ? 0 : 1;
}
''', encoding="utf-8"
        )
        executable = temporary / "check.exe"
        flags = ["gcc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
        subprocess.run(flags + [str(harness), "-o", str(executable)], check=True)
        sample = temporary / "sample.exe"
        subprocess.run(flags + [source, "-o", str(sample)], check=True)
        output = subprocess.run([str(sample)], capture_output=True, text=True, check=True)
        assert [int(line.split()[1]) for line in output.stdout.splitlines()[1:]] == [0, -1, 2, -2, 1]

        cases = [
            (1, [], 0),
            (3, [], 1),
            (2, [(0, 1, 0)], 0),
            (1, [(0, 0, -1)], 0),
            (3, [(1, 2, -2), (2, 1, 1)], 0),
            (3, [(0, 1, 2147483647), (1, 2, 2147483647)], 0),
            (3, [(0, 1, -2147483648), (1, 2, -2147483648)], 0),
            (3, [(0, 1, 5), (0, 1, 2), (1, 2, -1)], 0),
        ]
        generator = random.Random(42)
        for _ in range(100):
            vertices = generator.randint(1, 12)
            edges = [
                (generator.randrange(vertices), generator.randrange(vertices), generator.randint(-10, 20))
                for _ in range(generator.randint(0, vertices * 3))
            ]
            cases.append((vertices, edges, generator.randrange(vertices)))

        def run(vertices, edges, source):
            text = f"{vertices} {len(edges)} {source}\n"
            text += "".join(f"{a} {b} {w}\n" for a, b, w in edges)
            return subprocess.run([str(executable)], input=text, text=True, capture_output=True, timeout=5)

        for vertices, edges, source in cases:
            expected = reference(vertices, edges, source)
            result = run(vertices, edges, source)
            assert result.returncode == 0, result.stderr
            if expected is None:
                assert result.stdout.strip() == "Graph contains negative weight cycle"
            else:
                actual = [
                    None if line.split()[1] == "INF" else int(line.split()[1])
                    for line in result.stdout.splitlines()[1:]
                ]
                assert actual == expected, (vertices, edges, source, actual, expected)

        for case in [(0, [], 0), (2, [], -1), (2, [], 2), (2, [(0, 2, 1)], 0)]:
            assert run(*case).returncode == 1
        print(f"Passed sample output, {len(cases)} reference comparisons, and invalid-input checks.")


if __name__ == "__main__":
    main()
