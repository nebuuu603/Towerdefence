#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QWidget>
#include <QTimer>
#include <QVector>
#include <QPointF>
#include <QDateTime>
#include <QMenu>

enum TowerType { ATTACK_TOWER, FREEZE_TOWER };
enum GameState { MENU, PLAYING, LEVEL_COMPLETED, VICTORY, GAME_OVER };

struct Bullet {
    QPointF startPos;
    QPointF targetPos;
    qint64 startTime;
};

struct Level {
    int totalEnemies;
    int spawnInterval;
    int enemySpeed;
    int enemyHealth;
    QVector<QPointF> path;
    QVector<QPointF> buildZones;
};

class GameWindow : public QWidget
{
    Q_OBJECT

public:
    explicit GameWindow(QWidget* parent = nullptr);
    ~GameWindow() override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private slots:
    void updateGame();
    void handleTowerAttacks();
    void spawnEnemy();

private:
    void initLevels();
    void startLevel(int lv);
    void resetGame();

    // 游戏状态变量
    GameState currentState;
    int currentLevel;
    int playerHealth;
    int playerMoney;
    int enemiesSpawned;
    QRect nextLevelBtn;

    // 游戏对象容器
    QVector<QPointF> enemies;
    QVector<int> enemyHealth;
    QVector<int> enemyPathIndex;
    QVector<qint64> enemyFreezeEnd;    
    QVector<qint64> enemyLastFreezeTime; 
    QVector<QPointF> towers;
    QVector<TowerType> towerTypes;
    QVector<Bullet> bullets;

    // 定时器
    QTimer* updateTimer;
    QTimer* attackTimer;
    QTimer* spawnTimer;

    // 游戏常量
    const int SCREEN_W = 1280;
    const int SCREEN_H = 720;
    const int TOWER_COST = 300;
    const int TOWER_RANGE = 150;
    const int TOWER_DAMAGE = 20;
    const int PLAYER_MAX_HEALTH = 100;
    const int INITIAL_MONEY = 1100;
    const int FREEZE_DURATION = 5000;  
    const int FREEZE_PROTECTION = 15000; 
    const int BULLET_SPEED = 5;
    QVector<Level> levels;
};

#endif // GAMEWINDOW_H
