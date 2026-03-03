//alpha beta search function

int alphaBeta(Board& board, int depth, int alpha, int beta, bool maximizingPlayer) {
    if (depth == 0) {
        return evaluate(board); // Static evaluation (material + position)
    }

    auto moves = generateLegalMoves(board);
    if (moves.empty()) return isCheck(board) ? -10000 : 0; // Checkmate or Stalemate

    if (maximizingPlayer) {
        int maxEval = -1000000;
        for (auto& move : moves) {
            board.makeMove(move);
            int eval = alphaBeta(board, depth - 1, alpha, beta, false);
            board.unmakeMove(move);
            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);
            if (beta <= alpha) break; // Beta cut-off (Pruning)
        }
        return maxEval;
    } else {
        int minEval = 1000000;
        for (auto& move : moves) {
            board.makeMove(move);
            int eval = alphaBeta(board, depth - 1, alpha, beta, true);
            board.unmakeMove(move);
            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);
            if (beta <= alpha) break; // Alpha cut-off (Pruning)
        }
        return minEval;
    }
}