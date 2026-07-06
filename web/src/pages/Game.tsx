import { useEffect, useState } from "react";
import { Chess } from "chess.js";
import { Chessboard } from "react-chessboard";
import { GAME_TIMES, type GameMode } from "../types/GameModes";
import { useParams } from 'react-router-dom';
import { ENGINE_URL } from "../types/Constants";

async function initEngine(fen: string) {
  await fetch(`${ENGINE_URL}/position`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ fen }),
  });
}

async function getEngineMove(): Promise<{ move: string; fen: string } | null> {
  const res = await fetch(`${ENGINE_URL}/bestmove?depth=5`);
  if (!res.ok) return null;
  return res.json();
}

function Game() {
  const { mode } = useParams<{ mode: GameMode }>();

  if (!mode || !(mode in GAME_TIMES)) {
    return <div>Invalid game mode</div>;
  }

  const initialTime = GAME_TIMES[mode];

  const [game, setGame] = useState(new Chess());
  const [selectedSquare, setSelectedSquare] = useState<string | null>(null);
  const [whiteTime, setWhiteTime] = useState(initialTime);
  const [blackTime, setBlackTime] = useState(initialTime);
  const [gameStarted, setGameStarted] = useState(false);
  const [engineThinking, setEngineThinking] = useState(false);

  // Initialise engine on mount
  useEffect(() => {
    initEngine(game.fen());
  }, []);

  async function requestEngineMove(currentGame: Chess) {
    if (currentGame.isGameOver()) return;
    setEngineThinking(true);

    const result = await getEngineMove();
    if (result) {
      const updated = new Chess(result.fen);
      setGame(updated);
    }

    setEngineThinking(false);
  }

  function onDrop(sourceSquare: string, targetSquare: string) {
    if (engineThinking) return false;
  
    const gameCopy = new Chess(game.fen());
    const move = gameCopy.move({
      from: sourceSquare,
      to: targetSquare,
      promotion: "q",
    });
  
    if (move === null) return false;
  
    setGame(gameCopy);
    setGameStarted(true);
  
    // Fire async work separately — don't await here
    fetch(`${ENGINE_URL}/move`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ move: sourceSquare + targetSquare }),
    }).then(() => requestEngineMove(gameCopy));
  
    return true; // return synchronously
  }

  // click select
  function onSquareClick(square: string) {
    if (engineThinking) return;
  
    if (selectedSquare) {
      const gameCopy = new Chess(game.fen());
      const move = gameCopy.move({
        from: selectedSquare,
        to: square,
        promotion: "q",
      });
  
      if (move) {
        setGame(gameCopy);
        setGameStarted(true);
        setSelectedSquare(null);
  
        fetch(`${ENGINE_URL}/move`, {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ move: selectedSquare + square }),
        }).then(() => requestEngineMove(gameCopy));
  
        return;
      }
  
      setSelectedSquare(null);
    } else {
      setSelectedSquare(square);
    }
  }

  // turn indicator
  const turn = game.turn() === "w" ? "White" : "Black";
  let status = engineThinking ? "Engine thinking..." : `${turn}'s turn`;

  if (game.isCheckmate()) {
    status = `Checkmate! ${turn === "White" ? "Black" : "White"} wins`;
  } else if (game.isDraw()) {
    status = "Draw";
  } else if (game.isCheck()) {
    status = `${turn} is in check`;
  }

  // timer
  useEffect(() => {
    const interval = setInterval(() => {
      if (!gameStarted || game.isGameOver()) return;

      if (game.turn() === "w") {
        setWhiteTime((t) => {
          if (t <= 1) { clearInterval(interval); return 0; }
          return t - 1;
        });
      } else {
        setBlackTime((t) => {
          if (t <= 1) { clearInterval(interval); return 0; }
          return t - 1;
        });
      }
    }, 1000);

    return () => clearInterval(interval);
  }, [game, gameStarted]);

  function formatTime(seconds: number) {
    const mins = Math.floor(seconds / 60);
    const secs = seconds % 60;
    return `${mins}:${secs.toString().padStart(2, "0")}`;
  }

  async function reset() {
    const fresh = new Chess();
    setGame(fresh);
    setWhiteTime(initialTime);
    setBlackTime(initialTime);
    setGameStarted(false);
    setEngineThinking(false);
    await initEngine(fresh.fen());
  }

  return (
    <div className="game-wrapper">
      <div className="board-area">

        <div className="timer timer-top">
          <h3 className="black-timer">{formatTime(blackTime)}</h3>
        </div>

        <p>{status}</p>

        <Chessboard
          boardWidth={680}
          position={game.fen()}
          onPieceDrop={onDrop}
          onSquareClick={onSquareClick}
          arePiecesDraggable={!engineThinking}
        />

        <div className="timer timer-bottom">
          <h3 className="white-timer">{formatTime(whiteTime)}</h3>
        </div>

        <button onClick={reset}>Reset Game</button>

      </div>
    </div>
  );
}

export default Game;