#include <graphics.h>
#include<vector>
int idx_current_anim = 0;
int PLAYER_SPEED = 20;

bool is_game_started = false;
bool running = true;
const int PLAYER_ANIM_NUM = 1;

IMAGE img_player_left;
IMAGE img_player_right;

POINT player_pos = { 500,500 };

#pragma comment(lib,"Winmm.lib")
#pragma comment(lib,"MSIMG32.LIB")

inline void putimage_alpha(int x, int y, IMAGE* img)
{
    int w = img->getwidth();
    int h = img->getheight();
    AlphaBlend(GetImageHDC(NULL), x, y, w, h,
        GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER,0,255,AC_SRC_ALPHA });
}

class Animation
{
public:
    Animation(LPCTSTR path, int num, int interval)
    {
        interval_ms = interval;

        TCHAR path_file[256];
        for (size_t i = 0;i < num;i++)
        {
            _stprintf_s(path_file, path, i);

            IMAGE* frame = new IMAGE();
            loadimage(frame, path_file);
            frame_list.push_back(frame);
        }
    }
    ~Animation()
    {
        for (size_t i = 0;i < frame_list.size();i++)
        {
            delete frame_list[i];
        }
    }

    void Play(int x, int y, int delta)
    {
        timer += delta;
        if (timer >= interval_ms)
        {
            idx_frame = (idx_frame + 1) % frame_list.size();
            timer = 0;
        }

        putimage_alpha(x, y, frame_list[idx_frame]);
    }
    std::vector<IMAGE*>frame_list;
    int interval_ms = 0;
    int timer = 0;
    int idx_frame = 0;
};
class Player
{
public:
    Player()
    {
        anim_left = new Animation(_T("img/player_left_%d.png"), 1, 45);
        anim_right = new Animation(_T("img/player_right_%d.png"), 1, 45);
    }
    int FRAME_WIDTH = 80;
    int FRAME_HEIGHT = 80;

    ~Player()
    {
        delete anim_left;
        delete anim_right;
    }

    int dir_x = 0;

    void ProcessEvent(ExMessage& msg)
    {
        if (msg.message == WM_KEYDOWN)
        {
            switch (msg.vkcode)
            {
            case VK_UP:
                is_move_up = true;
                break;
            case VK_DOWN:
                is_move_down = true;
                break;
            case VK_LEFT:
                is_move_left = true;
                break;
            case VK_RIGHT:
                is_move_right = true;
                break;
            }
        }
        else if (msg.message == WM_KEYUP)
        {
            switch (msg.vkcode)
            {
            case VK_UP:
                is_move_up = false;
                break;
            case VK_DOWN:
                is_move_down = false;
                break;
            case VK_LEFT:
                is_move_left = false;
                break;
            case VK_RIGHT:
                is_move_right = false;
                break;
            }
        }

        if (is_move_up)
        {
            position.y -= PLAYER_SPEED;
        }
        if (is_move_down)
        {
            position.y += PLAYER_SPEED;
        }
        if (is_move_left)
        {
            position.x -= PLAYER_SPEED;
            dir_x = -1;
        }
        if (is_move_right)
        {
            position.x += PLAYER_SPEED;
            dir_x = 1;
        }
    }

    void Draw(int delta)
    {
        static bool facing_left = false;
        if (dir_x < 0)
            facing_left = true;
        else if (dir_x > 0)
            facing_left = false;
        if (position.y > 500)
        {
            position.y = 500;
        }
        if (facing_left)
        {
            anim_left->Play(position.x, position.y, delta);
        }
        else
        {
            anim_right->Play(position.x, position.y, delta);
        }
    }

    const POINT& GetPosition() const
    {
        return position;
    }

private:
    Animation* anim_left;
    Animation* anim_right;
    POINT position = { 500,500 };
    bool is_move_up = false;
    bool is_move_down = false;
    bool is_move_left = false;
    bool is_move_right = false;
};
class Bullet
{
public:
    POINT position = { 0,0 };

public:
    Bullet() = default;
    ~Bullet() = default;
    void Draw() const
    {
        setlinecolor(RGB(225, 155, 50));
        setfillcolor(RGB(200, 75, 10));
        fillcircle(position.x, position.y, RADIUS);
    }
private:
    const int RADIUS = 10;

};
class Enemy
{
public:
    Enemy()
    {
        loadimage(&img_shadow, _T("img/shadow_enemy.png"));
        anim_left = new Animation(_T("img/enemy_left_%d.png"), 6, 45);
        anim_right = new Animation(_T("img/enemy_right_%d.png"), 6, 45);

        enum class SpawnEdge
        {
            Up = 0,
            Down,
            Left,
            Right
        };

        SpawnEdge edge = (SpawnEdge)(rand() % 4);
        switch (edge)
        {
        case SpawnEdge::Up:
            position.x = rand() % 1280;
            position.y = -FRAME_HEIGHT;
            break;
        case SpawnEdge::Down:
            position.x = rand() % 1280;
            position.y = 720;
            break;

        case SpawnEdge::Left:
            position.x = -FRAME_WIDTH;
            position.y = rand() % 720;
            break;
        case SpawnEdge::Right:
            position.x = 1280;
            position.y = rand() % 720;
            break;
        default:
            break;
        }
    }
    bool CheckPlayerCollision(const Player& player)
    {
        POINT check_position = { position.x + FRAME_WIDTH / 2,position.y + FRAME_HEIGHT / 2 };
        POINT player_position = player.GetPosition();

        bool is_overlap_x = check_position.x >= player_position.x && check_position.x <= player_position.x + FRAME_WIDTH;
        bool is_overlap_y = check_position.y >= player_position.y && check_position.y <= player_position.y + FRAME_WIDTH;

        return is_overlap_x && is_overlap_y;
    }

    bool CheckBulletCollision(const Bullet& bullet)
    {
        bool is_overlap_x = bullet.position.x >= position.x && bullet.position.x <= position.x + FRAME_WIDTH;
        bool is_overlap_y = bullet.position.y >= position.y && bullet.position.y <= position.y + FRAME_WIDTH;
        return is_overlap_x && is_overlap_y;
    }

    void Move(const Player& player)
    {
        const POINT& player_position = player.GetPosition();
        int dir_x = player_position.x - position.x;
        int dir_y = player_position.y - position.y;
        double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);
        if (len_dir != 0)
        {
            double normalized_x = dir_x / len_dir;
            double normalized_y = dir_y / len_dir;
            position.x += (int)(SPEED * normalized_x);
            position.y += (int)(SPEED * normalized_y);
        }
        if (dir_x < 0)
            facing_left = true;
        else if (dir_x > 0)
            facing_left = false;
    }
    void Draw(int delta)
    {
        if (facing_left)
            anim_left->Play(position.x, position.y, delta);
        else
            anim_right->Play(position.x, position.y, delta);
    }
    ~Enemy()
    {
        delete anim_left;
        delete anim_right;
    }
    void Hurt()
    {
        alive = false;
    }

    bool CheckAlive()
    {
        return alive;
    }
private:
    const int SPEED = 2;
    const int FRAME_WIDTH = 80;
    const int FRAME_HEIGHT = 80;
    const int SHADOW_WIDTH = 48;
private:
    IMAGE img_shadow;
    Animation* anim_left;
    Animation* anim_right;
    POINT position = { 0,0 };
    bool facing_left = false;
    bool alive = true;
};
class Button
{
public:
    Button( RECT rect,LPCTSTR path_img_idle,LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
    {
        region = rect;

        loadimage(&img_idle, path_img_idle);
        loadimage(&img_hovered, path_img_hovered);
        loadimage(&img_pushed, path_img_pushed);
    }

    ~Button() = default;

    void ProcessEvent(const ExMessage& msg)
    {
        switch (msg.message)
        {
        case WM_MOUSEMOVE:
            if (status == Status::Idle && CheckCursorHit(msg.x, msg.y))
                status = Status::Hovered;
            else if (status == Status::Hovered && !CheckCursorHit(msg.x, msg.y))
                status = Status::Idle;
            break;
        case WM_LBUTTONDOWN:
            if (CheckCursorHit(msg.x, msg.y))
                status = Status::Pushed;
            break;
        case WM_LBUTTONUP:
            if (status == Status::Pushed)
                OnClick();
            break;
        default:
            break;
        }

    }
    
    void Draw()
    {
        switch (status)
        {
        case Status::Idle:
            putimage(region.left, region.top, &img_idle);
            break;
        case Status::Hovered:
            putimage(region.left, region.top, &img_hovered);
            break;
        case Status::Pushed:
            putimage(region.left, region.top, &img_pushed);
            break;
        }
    }

protected:
    virtual void OnClick() = 0;

private:
    enum class Status
    {
    Idle = 0,
    Hovered,
    Pushed
    };

private:
    RECT region;
    IMAGE img_idle;
    IMAGE img_hovered;
    IMAGE img_pushed;
    Status status=Status::Idle;
private:
    bool CheckCursorHit(int x, int y)
    {
        return x >= region.left && x <= region.right && y >= region.top && y <= region.bottom;
    }
};
class StartGameButton :public Button
{
public:
    StartGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
        :Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {}
    ~StartGameButton() = default;

protected:
    void OnClick()
    {
        is_game_started = true;

        mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);
    }
};
class QuitGameButton :public Button
{
public:
    QuitGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
        :Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {
    }
    ~QuitGameButton() = default;

protected:
    void OnClick()
    {
        running = false;
    }
};




void TryGenerateEnemy(std::vector<Enemy*>& enemy_list)
{
    const int INTERVAL = 100;
    static int counter = 0;
    if ((++counter) % INTERVAL == 0)
        enemy_list.push_back(new Enemy());

}
void UpdateBullets(std::vector<Bullet>& bullet_list, const Player& player)
{
    const double RADIAL_SPEED = 0.0045;
    const double TANGENT_SPEED = 0.0055;
    double radian_interval = 2 * 3.14159 / bullet_list.size();
    POINT player_position = player.GetPosition();
    double radius = 100 + 25 * sin(GetTickCount() * RADIAL_SPEED);
    for (size_t i = 0;i < bullet_list.size();i++)
    {
        double radian = GetTickCount() * TANGENT_SPEED + radian_interval * i;
        bullet_list[i].position.x = player_position.x + player.FRAME_WIDTH / 2 + (int)(radius * sin(radian));
        bullet_list[i].position.y = player_position.y + player.FRAME_HEIGHT / 2 + (int)(radius * cos(radian));
    }

}
void DrawPlayerScore(int score)
{
    static TCHAR text[64];
    _stprintf_s(text, _T("��ǰ��ҵ÷�:%d"), score);

    setbkmode(TRANSPARENT);
    settextcolor(RGB(255, 85, 185));
    outtextxy(10, 10, text);
}
int main()
{
    int delta = 0;
    initgraph(1280, 720);

    mciSendString(_T("open mus/bgm.mp3 alias bgm"), NULL, 0, NULL);
    mciSendString(_T("open mus/hit.wav alias hit"), NULL, 0, NULL);

    int score = 0;
    ExMessage msg;
    IMAGE img_background;
    IMAGE img_menu;
    std::vector<Enemy*> enemy_list;
    std::vector<Bullet> bullet_list(3);

    RECT region_btn_strat_game, region_btn_quit_game;

    region_btn_strat_game.left = (1280 - 192) / 2;
    region_btn_strat_game.right = region_btn_strat_game.left + 192;
    region_btn_strat_game.top = 430;
    region_btn_strat_game.bottom = region_btn_strat_game.top + 75;

    region_btn_quit_game.left = (1280 - 192) / 2;
    region_btn_quit_game.right = region_btn_strat_game.left + 192;
    region_btn_quit_game.top = 550;
    region_btn_quit_game.bottom = region_btn_strat_game.top + 75;

    StartGameButton btn_start_game = StartGameButton(region_btn_strat_game, _T("img/ui_start_idle.png"), _T("img/ui_start_hovered.png"), _T("img/ui_start_pushed.png"));
    QuitGameButton btn_quit_game = QuitGameButton(region_btn_quit_game, _T("img/ui_quit_idle.png"), _T("img/ui_quit_hovered.png"), _T("img/ui_quit_pushed.png"));

    loadimage(&img_background, _T("img/background.png"));
    loadimage(&img_menu, _T("img/menu.png"));


    BeginBatchDraw();

    Player player;

    while (running)
    {
        DWORD start_time = GetTickCount();

        while (peekmessage(&msg))
        {
            if(is_game_started)
            player.ProcessEvent(msg);
            else
            {
                btn_start_game.ProcessEvent(msg);
                btn_quit_game.ProcessEvent(msg);
            }
        }

        UpdateBullets(bullet_list, player);

        if (is_game_started)
        {
            TryGenerateEnemy(enemy_list);

            for (Enemy* enemy : enemy_list)
                enemy->Move(player);

            for (Enemy* enemy : enemy_list)
            {
                if (enemy->CheckPlayerCollision(player))
                {
                    static TCHAR text[128];
                    _stprintf_s(text, _T("���յ÷֣�%d !"), score);
                    MessageBox(GetHWnd(), _T("�ۡ�1���ۿ�ս��CG"), _T("��Ϸ����"), MB_OK);
                    running = false;
                    break;
                }
            }

            for (Enemy* enemy : enemy_list)
            {
                for (const Bullet& bullet : bullet_list)
                {
                    if (enemy->CheckBulletCollision(bullet))
                    {
                        mciSendString(_T("play hit from 0"), NULL, 0, NULL);
                        enemy->Hurt();
                        score++;
                    }
                }
            }

            for (size_t i = 0;i < enemy_list.size();i++)
            {
                Enemy* enemy = enemy_list[i];
                if (!enemy->CheckAlive())
                {
                    std::swap(enemy_list[i], enemy_list.back());
                    enemy_list.pop_back();
                    delete enemy;
                }
            }
        }
        cleardevice();
        if (is_game_started)
        {
            putimage(0, 0, &img_background);
            player.Draw(1000 / 144);
            for (Enemy* enemy : enemy_list)
                enemy->Draw(1000 / 144);
            for (const Bullet& bullet : bullet_list)
                bullet.Draw();
            DrawPlayerScore(score);
        }
        else
        {
            putimage(0, 0, &img_menu);
            btn_start_game.Draw();
            btn_quit_game.Draw();
        }
        FlushBatchDraw();

        DWORD end_time = GetTickCount();
        DWORD delta_time = end_time - start_time;
        if (delta_time < 1000 / 144)
        {
            Sleep(1000 / 144 - delta_time);
        }
    }

    EndBatchDraw();

    return 0;
}