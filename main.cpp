#include <Novice.h>
#define _USE_MATH_DEFINES
#include <math.h>

const char kWindowTitle[] = "ジャストガードの実装";

struct Vector2 {
  float x;
  float y;
};

struct Player {
  Vector2 pos;
  Vector2 velocity;
  float radius;
  unsigned int color;
};

struct Enemy {
  Vector2 pos;
  Vector2 velocity;
  float radius;
  int isAttacking;
  int isKnockback;            // ノックバック中かどうか
  float knockbackDistance;    // ノックバックした距離の累計
  Vector2 knockbackDirection; // ノックバック方向
};

// 敵の追従
void UpdateEnemyChase(const Player *player, Enemy *enemy, float speed) {
  Vector2 direction;
  direction.x = player->pos.x - enemy->pos.x;
  direction.y = player->pos.y - enemy->pos.y;

  float length = sqrtf(direction.x * direction.x + direction.y * direction.y);
  if (length != 0.0f) {
    direction.x /= length;
    direction.y /= length;
  }

  enemy->velocity.x = direction.x * speed;
  enemy->velocity.y = direction.y * speed;

  enemy->pos.x += enemy->velocity.x;
  enemy->pos.y += enemy->velocity.y;
}

// 当たり判定
int IsCollision(const Player *player, const Enemy *enemy) {
  if (sqrtf(powf(player->pos.x - enemy->pos.x, 2) +
            powf(player->pos.y - enemy->pos.y, 2)) <
      player->radius + enemy->radius) {
    return true;
  } else {
    return false;
  }
}

// ノックバック処理更新
void UpdateKnockback(Enemy *enemy, float speed, float maxDistance) {
  if (!enemy->isKnockback) {
    return;
  }

  Vector2 displacement = {enemy->knockbackDirection.x * speed,
                          enemy->knockbackDirection.y * speed};

  enemy->pos.x += displacement.x;
  enemy->pos.y += displacement.y;
  enemy->knockbackDistance +=
      sqrtf(powf(displacement.x, 2) + powf(displacement.y, 2));

  // 規定距離まで吹っ飛ばしたら停止
  if (enemy->knockbackDistance >= maxDistance) {
    enemy->isKnockback = false;
    enemy->velocity = {0, 0};
  }
}

// ジャストガード判定
int TryJustGuard(Player *player, Enemy *enemy, const char *keys,
                 const char *preKeys, int frameInRange,
                 int justGuardFrameWindow) {
  if (!enemy->isAttacking) {
    return false;
  }

  // 一定距離近付いたときに判定を開始
  if (sqrtf(powf(player->pos.x - enemy->pos.x, 2) +
            powf(player->pos.y - enemy->pos.y, 2)) <
      player->radius + enemy->radius + 2.0f) {

    // ジャストガード受付時間内だったとき
    if (frameInRange <= justGuardFrameWindow) {
      // スペースキーをトリガー
      if (keys[DIK_SPACE] && !preKeys[DIK_SPACE]) {

        // 敵を吹っ飛ばす
        Vector2 direction = {enemy->pos.x - player->pos.x,
                             enemy->pos.y - player->pos.y};

        float length = sqrtf(powf(direction.x, 2) + powf(direction.y, 2));

        if (length != 0.0f) {
          direction.x /= length;
          direction.y /= length;
        }

        enemy->knockbackDirection = direction;
        enemy->isKnockback = true;
        enemy->knockbackDistance = 0.0f;
        enemy->isAttacking = false;

        return true;
      }
    }
  }

  return false;
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
  // ライブラリの初期化
  Novice::Initialize(kWindowTitle, 1280, 720);

  // キー入力結果を受け取る箱
  char keys[256] = {0};
  char preKeys[256] = {0};

  Player player = {{360.0f, 360.0f}, {7.0f, 7.0f}, 64.0f, WHITE};
  Enemy enemy = {
      {800.0f, 360.0f}, {0.0f, 0.0f}, 64.0f, false, false, 0.0f, {0, 0}};

  int frameInRange = 0;

  const int justGuardFrameWindow = 2;
  float distance = 0.0f;

  // ウィンドウの×ボタンが押されるまでループ
  while (Novice::ProcessMessage() == 0) {
    // フレームの開始
    Novice::BeginFrame();

    // キー入力を受け取る
    memcpy(preKeys, keys, 256);
    Novice::GetHitKeyStateAll(keys);

    ///
    /// ↓更新処理ここから
    ///

    // プレイヤー移動
    if (keys[DIK_D])
      player.pos.x += player.velocity.x;
    if (keys[DIK_A])
      player.pos.x -= player.velocity.x;
    if (keys[DIK_S])
      player.pos.y += player.velocity.y;
    if (keys[DIK_W])
      player.pos.y -= player.velocity.y;

    // 敵初期化
    if (keys[DIK_F] && !preKeys[DIK_F]) {
      enemy.isAttacking = true;
      enemy.isKnockback = false;
      enemy.pos = {800.0f, 360.0f};
    }

    distance = sqrtf(powf(player.pos.x - enemy.pos.x, 2) +
                     powf(player.pos.y - enemy.pos.y, 2));

    // 敵の移動
    if (enemy.isAttacking) {
      UpdateEnemyChase(&player, &enemy, 7.0f);

      // ジャストガード受付判定開始
      if (distance < player.radius + enemy.radius + 2.0f) {
        // 一定範囲内のときフレームカウントを増やす
        frameInRange++;
      } else {
        frameInRange = 0;
      }
    }

    // 衝突時の色変化
    if (IsCollision(&player, &enemy)) {
      player.color = RED;
    } else {
      if (!enemy.isKnockback) {
        player.color = WHITE;
      }
    }

    // ジャストガード実行
    if (TryJustGuard(&player, &enemy, keys, preKeys, frameInRange,
                     justGuardFrameWindow)) {
      player.color = 0x00FFFFFF;
    }

    // ジャストガードできたとき
    if (enemy.isKnockback) {
      // ノックバック更新
      UpdateKnockback(&enemy, 10.0f, 200.0f);
    }

    ///
    /// ↑更新処理ここまで
    ///

    ///
    /// ↓描画処理ここから
    ///

    // 敵の描画
    Novice::DrawEllipse(
        static_cast<int>(enemy.pos.x), static_cast<int>(enemy.pos.y),
        static_cast<int>(enemy.radius), static_cast<int>(enemy.radius), 0.0f,
        BLUE, kFillModeSolid);

    // プレイヤーの描画
    Novice::DrawEllipse(
        static_cast<int>(player.pos.x), static_cast<int>(player.pos.y),
        static_cast<int>(player.radius), static_cast<int>(player.radius), 0.0f,
        player.color, kFillModeSolid);

    if (enemy.isAttacking) {
      // 速めに表示
      if (distance < player.radius + enemy.radius + 90.0f) {
        Novice::ScreenPrintf(400, 600, "JustGuard!!!");
      }
    }

    Novice::ScreenPrintf(10, 30, "koutai: %d", enemy.isKnockback);
    Novice::ScreenPrintf(10, 60, "frameInRange : justGuardFrameWindow");
    Novice::ScreenPrintf(10, 80, "%d : %d", frameInRange, justGuardFrameWindow);

    ///
    /// ↑描画処理ここまで
    ///

    // フレームの終了
    Novice::EndFrame();

    // ESCキーが押されたらループを抜ける
    if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
      break;
    }
  }

  // ライブラリの終了
  Novice::Finalize();
  return 0;
}
