from .utils import chess_manager, GameContext
from chess import Move

from subprocess import Popen, PIPE, run
import requests

# Write code here that runs once
# Can do things like load models from huggingface, make connections to subprocesses, etcwenis

with open("src/engine/nnue.bin", "wb") as f:
    res = requests.get("https://pgn.int0x80.ca/nnue.bin")
    f.write(res.content)

run(["make", "-C", "src/engine", "-j"])

p = Popen("src/a.out", stdin=PIPE, stdout=PIPE, text=True)
p.stdin.write("uci\n")
p.stdin.flush()

@chess_manager.entrypoint
def test_func(ctx: GameContext):
    # This gets called every time the model needs to make a move
    # Return a python-chess Move object that is a legal move for the current position

    p.stdin.write(f"position fen {ctx.board.fen()}\n")
    p.stdin.write(f"go movetime {8000}\n")
    p.stdin.flush()

    while True:
        line = p.stdout.readline().strip()
        print(line)
        if line.startswith("bestmove"):
            parts = line.split()
            bestmove = parts[1]
            return Move.from_uci(bestmove)


@chess_manager.reset
def reset_func(ctx: GameContext):
    p.stdin.write("ucinewgame\n")
    p.stdin.flush()
