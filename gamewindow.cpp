#include "gamewindow.h"
#include <QPainter>
#include <QMouseEvent>
#include <QLineF>
#include <cmath>
#include <QDebug>

inline double distance(const QPointF& a, const QPointF& b) {
    return QLineF(a, b).length();
}

GameWindow::GameWindow(QWidget* parent)
    : QWidget(parent),
    currentState(MENU),
    currentLevel(0),
    playerHealth(PLAYER_MAX_HEALTH),
    playerMoney(INITIAL_MONEY),
    enemiesSpawned(0),
    nextLevelBtn(SCREEN_W / 2 - 150, SCREEN_H / 2 + 50, 300, 60),
    enemyFreezeEnd(),
    enemyLastFreezeTime(),
    towers(),
    towerTypes(),
    bullets()
{
    setFixedSize(SCREEN_W, SCREEN_H);
    setWindowTitle("塔防游戏");

    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &GameWindow::updateGame);
    updateTimer->start(30);

    attackTimer = new QTimer(this);
    connect(attackTimer, &QTimer::timeout, this, &GameWindow::handleTowerAttacks);
    attackTimer->start(1000);

    spawnTimer = new QTimer(this);
    connect(spawnTimer, &QTimer::timeout, this, &GameWindow::spawnEnemy);

    initLevels();
}

GameWindow::~GameWindow() {
    delete updateTimer;
    delete attackTimer;
    delete spawnTimer;
}

void GameWindow::initLevels() {
    // 关卡1：8字波浪路径
    Level lvl1;
    lvl1.totalEnemies = 12;
    lvl1.spawnInterval = 1600;
    lvl1.enemySpeed = 2;
    lvl1.enemyHealth = 100;
    lvl1.path = {
        QPointF(-50, 360), QPointF(200, 200), QPointF(400, 520),
        QPointF(600, 200), QPointF(800, 520), QPointF(1000, 360), QPointF(1300, 360)
    };
    lvl1.buildZones = {
        QPointF(200, 200 - 80), QPointF(200, 200 + 80),
        QPointF(400, 520 - 80), QPointF(400, 520 + 80),
        QPointF(600, 200 - 80), QPointF(600, 200 + 80)
    };
    levels.append(lvl1);

    // 关卡2：双环交叉路径
    Level lvl2;
    lvl2.totalEnemies = 18;
    lvl2.spawnInterval = 1300;
    lvl2.enemySpeed = 3;
    lvl2.enemyHealth = 150;
    lvl2.path = {
        QPointF(-50, 360), QPointF(300, 150), QPointF(500, 150),
        QPointF(600, 360), QPointF(500, 570), QPointF(300, 570),
        QPointF(200, 360), QPointF(400, 360), QPointF(1300, 360)
    };
    lvl2.buildZones = { QPointF(400, 360), QPointF(400, 150), QPointF(400, 570) };
    levels.append(lvl2);

    // 关卡3：三层立体路径
    Level lvl3;
    lvl3.totalEnemies = 25;
    lvl3.spawnInterval = 1000;
    lvl3.enemySpeed = 4;
    lvl3.enemyHealth = 220;
    lvl3.path = {
        QPointF(-50, 360), QPointF(200, 120), QPointF(400, 120),
        QPointF(500, 360), QPointF(700, 360), QPointF(800, 600),
        QPointF(1000, 600), QPointF(1300, 360)
    };
    lvl3.buildZones = {
        QPointF(300, 120), QPointF(600, 360), QPointF(900, 600),
        QPointF(450, 240), QPointF(750, 480)
    };
    levels.append(lvl3);
}

void GameWindow::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    if (currentState == MENU) {
        p.fillRect(0, 0, SCREEN_W, SCREEN_H, QColor(30, 30, 30));
        p.setPen(Qt::white);
        p.setFont(QFont("微软雅黑", 48, QFont::Bold));
        p.drawText(rect().adjusted(0, -200, 0, 0), Qt::AlignCenter, "塔防游戏");

        QRect startBtn(SCREEN_W / 2 - 200, SCREEN_H / 2 - 50, 400, 80);
        p.setPen(Qt::white);
        p.drawRoundedRect(startBtn, 10, 10);
        p.drawText(startBtn, Qt::AlignCenter, "开始游戏");

        QRect exitBtn(SCREEN_W / 2 - 200, SCREEN_H / 2 + 50, 400, 80);
        p.drawRoundedRect(exitBtn, 10, 10);
        p.drawText(exitBtn, Qt::AlignCenter, "退出游戏");
    }
    else if (currentState == PLAYING) {
        p.fillRect(0, 0, SCREEN_W, SCREEN_H, QColor(240, 240, 240));

        // 绘制路径
        p.setPen(QPen(QColor(120, 120, 120), 35));
        const auto& path = levels[currentLevel - 1].path;
        for (int i = 0; i < path.size() - 1; ++i)
            p.drawLine(path[i], path[i + 1]);

        // 绘制敌人
        p.setBrush(Qt::red);
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        for (int i = 0; i < enemies.size(); ++i) {
            QPointF pos = enemies[i];
            p.drawEllipse(pos, 12, 12);  

            if (enemyFreezeEnd[i] > currentTime) {
                p.setPen(QPen(Qt::blue, 2, Qt::DashLine));
                p.drawEllipse(pos, 20, 20);  
                p.setPen(Qt::NoPen);
            }
        }

        // 绘制防御塔
        for (int i = 0; i < towers.size(); ++i) {
            QPointF pos = towers[i];
            QColor color = (towerTypes[i] == ATTACK_TOWER) ? Qt::blue : QColor(120, 60, 240);
            p.setBrush(color);
            p.drawRoundedRect(pos.x() - 20, pos.y() - 20, 40, 40, 5, 5);
        }

        // 绘制建造区（黄色虚线框）
        p.setPen(QPen(Qt::yellow, 2, Qt::DashLine));
        for (const auto& zone : levels[currentLevel - 1].buildZones)
            p.drawRoundedRect(zone.x() - 25, zone.y() - 25, 50, 50, 5, 5);

        // 绘制HUD（金钱/血量/关卡）
        p.setPen(Qt::black);
        p.setFont(QFont("微软雅黑", 18));
        p.drawText(20, 30, QString("金钱: $%1").arg(playerMoney));
        p.drawText(20, 60, QString("生命: %1/%2").arg(playerHealth).arg(PLAYER_MAX_HEALTH));
        p.drawText(20, 90, QString("关卡: %1/3").arg(currentLevel));

        // 绘制子弹
        p.setPen(QPen(Qt::yellow, 3));
        for (const auto& bullet : bullets) {
            qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - bullet.startTime;
            double progress = qMin(elapsed / 500.0, 1.0);
            QPointF currentPos = bullet.startPos + (bullet.targetPos - bullet.startPos) * progress;
            p.drawLine(bullet.startPos, currentPos);
        }
    }
    else if (currentState == LEVEL_COMPLETED) {
        p.fillRect(0, 0, SCREEN_W, SCREEN_H, QColor(0, 0, 0, 150));
        p.setPen(Qt::green);
        p.setFont(QFont("微软雅黑", 48, QFont::Bold));
        p.drawText(rect().adjusted(0, -80, 0, 0), Qt::AlignCenter,
            QString("第%1关完成！").arg(currentLevel));

        p.setPen(Qt::white);
        p.drawRoundedRect(nextLevelBtn, 8, 8);
        p.setFont(QFont("微软雅黑", 24));
        p.drawText(nextLevelBtn, Qt::AlignCenter,
            currentLevel < 3 ? "进入下一关" : "挑战成功");
    }
    else if (currentState == VICTORY) {
        p.fillRect(0, 0, SCREEN_W, SCREEN_H, QColor(0, 0, 0, 150));
        p.setPen(Qt::green);
        p.setFont(QFont("微软雅黑", 48, QFont::Bold));
        p.drawText(rect(), Qt::AlignCenter, "所有关卡通关！\n恭喜获胜！");
    }
    else {
        p.fillRect(0, 0, SCREEN_W, SCREEN_H, QColor(0, 0, 0, 150));
        p.setPen(Qt::red);
        p.setFont(QFont("微软雅黑", 48, QFont::Bold));
        p.drawText(rect(), Qt::AlignCenter, "游戏失败！\n点击重试");
    }
}

void GameWindow::mousePressEvent(QMouseEvent* event) {
    QPoint pos = event->pos();
    if (currentState == MENU) {
        QRect startBtn(SCREEN_W / 2 - 200, SCREEN_H / 2 - 50, 400, 80);
        QRect exitBtn(SCREEN_W / 2 - 200, SCREEN_H / 2 + 50, 400, 80);
        if (startBtn.contains(pos)) startLevel(1);
        else if (exitBtn.contains(pos)) close();
    }
    else if (currentState == PLAYING) {
        if (event->button() == Qt::LeftButton) {
            const auto& zones = levels[currentLevel - 1].buildZones;
            for (const auto& zone : zones) {
                if (distance(zone, pos) < 30 && playerMoney >= TOWER_COST) {
                    bool hasTower = false;
                    for (const auto& tower : towers) {
                        if (distance(tower, zone) < 10) {
                            hasTower = true;
                            break;
                        }
                    }
                    if (!hasTower) {
                        towers.append(zone);
                        towerTypes.append(ATTACK_TOWER);
                        playerMoney -= TOWER_COST;
                        update();
                    }
                    break;
                }
            }
        }
        else if (event->button() == Qt::RightButton) {
            const auto& zones = levels[currentLevel - 1].buildZones;
            for (int i = 0; i < zones.size(); ++i) {
                if (distance(zones[i], pos) < 30 && playerMoney >= TOWER_COST) {
                    bool hasTower = false;
                    for (int j = 0; j < towers.size(); ++j) {
                        if (distance(towers[j], zones[i]) < 10) {
                            hasTower = true;
                            break;
                        }
                    }
                    if (!hasTower) {
                        QMenu towerMenu;
                        QAction* attackAction = towerMenu.addAction("攻击塔（蓝色）");
                        QAction* freezeAction = towerMenu.addAction("冰冻塔（紫色）");

                        QAction* selected = towerMenu.exec(mapToGlobal(pos));
                        if (selected == attackAction) {
                            towers.append(zones[i]);
                            towerTypes.append(ATTACK_TOWER);
                        }
                        else if (selected == freezeAction) {
                            towers.append(zones[i]);
                            towerTypes.append(FREEZE_TOWER);
                        }
                        playerMoney -= TOWER_COST;
                        update();
                    }
                    break;
                }
            }
        }
    }
    else if (currentState == LEVEL_COMPLETED) {
        // 关卡完成
        if (nextLevelBtn.contains(pos)) {
            if (currentLevel < 3) {
                startLevel(currentLevel + 1);  // 进入下一关
            }
            else {
                currentState = VICTORY;  // 所有关卡完成
            }
            update();
        }
    }
    else {
        // 游戏失败后重置
        resetGame();
    }
}

void GameWindow::updateGame() {
    if (currentState != PLAYING) return;

    const Level& lvl = levels[currentLevel - 1];
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    // 敌人移动逻辑
    for (int i = 0; i < enemies.size(); ++i) {
        if (enemyFreezeEnd[i] > currentTime) continue;

        int idx = enemyPathIndex[i];
        QPointF target = lvl.path[idx + 1];
        QPointF dir = target - enemies[i];
        double dist = distance(enemies[i], target);

        if (dist < 8) {
            if (idx + 1 < lvl.path.size() - 1) {
                enemyPathIndex[i]++;
            }
            else {
                playerHealth = qMax(playerHealth - 15, 0);
                enemies.remove(i);
                enemyHealth.remove(i);
                enemyPathIndex.remove(i);
                enemyFreezeEnd.remove(i);
                enemyLastFreezeTime.remove(i);
                i--;
            }
        }
        else {
            enemies[i] += dir / dist * lvl.enemySpeed;
        }
    }

    // 冰冻塔逻辑）
    for (int i = 0; i < towers.size(); ++i) {
        if (towerTypes[i] == FREEZE_TOWER) {
            QPointF towerPos = towers[i];
            for (int j = 0; j < enemies.size(); ++j) {
                
                bool inRange = (distance(towerPos, enemies[j]) < TOWER_RANGE);
                bool notFreezing = (enemyFreezeEnd[j] <= currentTime);
                bool canFreeze = (currentTime - enemyLastFreezeTime[j] > FREEZE_PROTECTION);

                if (inRange && notFreezing && canFreeze) {
                    
                    enemyFreezeEnd[j] = currentTime + FREEZE_DURATION;
                    enemyLastFreezeTime[j] = currentTime;
                }
            }
        }
    }

    // 清理过期子弹
    QVector<Bullet> activeBullets;
    for (const auto& bullet : bullets) {
        if (currentTime - bullet.startTime < 1000) {
            activeBullets.append(bullet);
        }
    }
    bullets = activeBullets;

    // 关卡检测
    bool allSpawned = (enemiesSpawned >= lvl.totalEnemies);
    bool noEnemiesLeft = enemies.isEmpty();
    if (allSpawned && noEnemiesLeft && playerHealth > 0) {
        currentState = LEVEL_COMPLETED;
    }
    if (playerHealth <= 0) currentState = GAME_OVER;

    update();
}

void GameWindow::handleTowerAttacks() {
    if (currentState != PLAYING) return;

    for (int i = 0; i < towers.size(); ++i) {
        if (towerTypes[i] == ATTACK_TOWER) {
            int target = -1;
            double minDist = TOWER_RANGE;
            for (int j = 0; j < enemies.size(); ++j) {
                double dist = distance(towers[i], enemies[j]);
                if (dist < minDist) {
                    minDist = dist;
                    target = j;
                }
            }
            if (target != -1) {
                Bullet bullet;
                bullet.startPos = towers[i];
                bullet.targetPos = enemies[target];
                bullet.startTime = QDateTime::currentMSecsSinceEpoch();
                bullets.append(bullet);

                enemyHealth[target] -= TOWER_DAMAGE;
                if (enemyHealth[target] <= 0) {
                    enemies.remove(target);
                    enemyHealth.remove(target);
                    enemyPathIndex.remove(target);
                    enemyFreezeEnd.remove(target);
                    enemyLastFreezeTime.remove(target);
                    playerMoney += 50;
                }
            }
        }
    }
}

void GameWindow::spawnEnemy() {
    if (enemiesSpawned >= levels[currentLevel - 1].totalEnemies) {
        spawnTimer->stop();
        return;
    }
    enemies.append(levels[currentLevel - 1].path[0]);
    enemyHealth.append(levels[currentLevel - 1].enemyHealth);
    enemyPathIndex.append(0);
    enemyFreezeEnd.append(0);      
    enemyLastFreezeTime.append(0); 
    enemiesSpawned++;
}

void GameWindow::startLevel(int lv) {
    if (lv < 1 || lv > 3) {
        currentState = GAME_OVER;
        return;
    }

    currentState = PLAYING;
    currentLevel = lv;
    playerHealth = PLAYER_MAX_HEALTH;
    playerMoney = INITIAL_MONEY;
    enemiesSpawned = 0;

    enemies.clear();
    enemyHealth.clear();
    enemyPathIndex.clear();
    enemyFreezeEnd.clear();
    enemyLastFreezeTime.clear();  // 重置敌人冻结记录
    towers.clear();
    towerTypes.clear();
    bullets.clear();

    spawnTimer->start(levels[lv - 1].spawnInterval);
}

void GameWindow::resetGame() {
    currentState = MENU;
    currentLevel = 0;
    playerHealth = PLAYER_MAX_HEALTH;
    playerMoney = INITIAL_MONEY;
    enemiesSpawned = 0;

    enemies.clear();
    enemyHealth.clear();
    enemyPathIndex.clear();
    enemyFreezeEnd.clear();
    enemyLastFreezeTime.clear();  // 重置敌人冻结记录
    towers.clear();
    towerTypes.clear();
    bullets.clear();

    update();
}
