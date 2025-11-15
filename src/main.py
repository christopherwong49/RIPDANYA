from .utils import chess_manager, GameContext
from chess import Move

from subprocess import Popen, PIPE

# Write code here that runs once
# Can do things like load models from huggingface, make connections to subprocesses, etcwenis

@chess_manager.entrypoint
def test_func(ctx: GameContext):
    # This gets called every time the model needs to make a move
    # Return a python-chess Move object that is a legal move for the current position

    p = Popen("/home/wdotmathree/uw/chess/PZChessBot/pzchessbot", stdin=PIPE, stdout=PIPE, text=True)

    p.stdin.write(f"uci\nposition fen {ctx.board.fen()}\n")
    p.stdin.write("go wtime 1000 btime 1000\n")
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
    pass
