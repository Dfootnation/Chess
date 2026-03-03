import { useEffect, useState } from "react";
import { Chess } from "chess.js";
import { Chessboard } from "react-chessboard";
import { GAME_TIMES, type GameMode } from "../types/GameModes";
import { useParams } from 'react-router-dom';

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

  // drag and drop
  function onDrop(sourceSquare: string, targetSquare: string) {
    const gameCopy = new Chess(game.fen());
    const move = gameCopy.move({
      from: sourceSquare,
      to: targetSquare,
      promotion: "q",
    });

    if (move === null) return false;

    setGame(gameCopy);
    setGameStarted(true);
    return true;
  }

  // click select
  function onSquareClick(square: string) {
    if (selectedSquare) {
      const move = game.move({
        from: selectedSquare,
        to: square,
        promotion: "q",
      });

      if (move) {
        setGame(new Chess(game.fen()));
        setGameStarted(true);
      }

      setSelectedSquare(null);
    } else {
      setSelectedSquare(square);
    }
  }

  // turn indicator
  const turn = game.turn() === "w" ? "White" : "Black";

  let status = `${turn}'s turn`;

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
          if (t <= 1) {
            status = "Black wins";
            clearInterval(interval);
            return 0;
          }
          return t - 1;
        });
      } else {
        setBlackTime((t) => {
          if (t <= 1) {
            status = "White wins";
            clearInterval(interval);
            return 0;
          }
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

  function reset() {
    setGame(new Chess());
    setWhiteTime(initialTime);
    setBlackTime(initialTime);
    setGameStarted(false);
  }

  return (
    <div className="game-wrapper">
      <div className="board-area">
  
        <div className="timer timer-top">
          <h3 className="black-timer">{formatTime(blackTime)}</h3>
        </div>
  
        <Chessboard
          boardWidth={680}
          position={game.fen()}
          onPieceDrop={onDrop}
          onSquareClick={onSquareClick}
        />
  
        <div className="timer timer-bottom">
          <h3 className="white-timer">{formatTime(whiteTime)}</h3>
        </div>
  
        <button onClick={() => reset()}>
          Reset Game
        </button>
  
      </div>
    </div>
  );
}

export default Game;