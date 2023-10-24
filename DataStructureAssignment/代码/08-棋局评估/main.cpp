#include <algorithm>
#include <iostream>
#include <cstdlib>

enum Slot {
    Empty = 0,
    ChessX,
    ChessO,
};

Slot board[3][3];

int calc(Slot to_place) {
    auto check_win_fn = []() {
        auto check_three_fn = [](int a, int b, int c) {
            return a == b && b == c && a != Empty;
        };
        if (check_three_fn(board[0][0], board[1][1], board[2][2])) {
            return board[0][0];
        }
        if (check_three_fn(board[0][2], board[1][1], board[2][0])) {
            return board[0][2];
        }
        if (check_three_fn(board[0][0], board[0][1], board[0][2])) {
            return board[0][0];
        }
        if (check_three_fn(board[1][0], board[1][1], board[1][2])) {
            return board[1][0];
        }
        if (check_three_fn(board[2][0], board[2][1], board[2][2])) {
            return board[2][0];
        }
        if (check_three_fn(board[0][0], board[1][0], board[2][0])) {
            return board[0][0];
        }
        if (check_three_fn(board[0][1], board[1][1], board[2][1])) {
            return board[0][1];
        }
        if (check_three_fn(board[0][2], board[1][2], board[2][2])) {
            return board[0][2];
        }
        return Empty;
    };
    auto check_win_result = check_win_fn();
    if (check_win_result != Empty) {
        int empty_cnt = 0;
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < 3; x++) {
                empty_cnt += board[y][x] == Empty;
            }
        }
        return (empty_cnt + 1) * (check_win_result == ChessX ? 1 : -1);
    }
    // DFS brute force
    const int MYINFINITY = 0xffff;
    int best_score = to_place == ChessX ? -MYINFINITY : MYINFINITY;
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            if (board[y][x] != Empty) { continue; }
            board[y][x] = to_place;
            if (to_place == ChessX) {
                best_score = std::max(best_score, calc(ChessO));
            }
            else {
                best_score = std::min(best_score, calc(ChessX));
            }
            board[y][x] = Empty;
        }
    }
    if (std::abs(best_score) == MYINFINITY) {
        // Draw
        return 0;
    }
    // Someone won, return its best score
    return best_score;
}

int main(void) {
    int n;
    std::cin >> n;
    for (int i = 0; i < n; i++) {
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < 3; x++) {
                std::cin >> reinterpret_cast<int&>(board[y][x]);
            }
        }
        std::cout << calc(ChessX) << std::endl;
    }
}
