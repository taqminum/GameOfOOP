#include<graphics.h>
#include<string>
#include<vector>
#include <cmath>


const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;

const int BUTTON_WIDTH = 190;
const int BUTTON_HEIGHT = 75;


#pragma comment(lib,"MSIMG32.LIB")//链接到这个库，为了图像处理（alphablend）
#pragma comment(lib,"Winmm.lib")//链接，为了音乐处理

bool is_game_started = false;
bool running = true;//这俩都是为了UI

inline void putimage_alpha(int x, int y, IMAGE* img, BYTE alpha = 255)
{
	BLENDFUNCTION bf = { AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA };
	AlphaBlend(GetImageHDC(NULL), x, y, img->getwidth(), img->getheight(),
		GetImageHDC(img), 0, 0, img->getwidth(), img->getheight(), bf);
}

class Atlas//Animation之间共享
{
	public:
		Atlas(LPCTSTR path, int num)
		{
			TCHAR path_file[256];
			for (size_t i = 1;i <= num;i++)
			{
				_stprintf_s(path_file, path, i);

				IMAGE* frame = new IMAGE();
				loadimage(frame, path_file);
				frame_list.push_back(frame);
			}
		}
		~Atlas()
		{
			for (size_t i = 0;i < frame_list.size();i++)
			{
				delete frame_list[i];
			}
		}
	public:
		std::vector<IMAGE*> frame_list;
};

Atlas* atlas_player_left;
Atlas* atlas_player_right;
Atlas* atlas_enemy_left;
Atlas* atlas_enmey_right;



class Animation
{
public:
	Animation(Atlas* atlas, int interval)//path是文件路径，num是动画的帧数,interval是帧之间的间隔.构造函数负责load
	{
		anim_atlas = atlas;
		interval_ms = interval;
	}

	~Animation() = default;

	void Play(int x, int y, int delta)
	{
		// 直接访问图集中的帧列表
		if (anim_atlas->frame_list.empty()) return;

		timer += delta;

		// 检查是否为最后一帧
		bool isLastFrame = (idx_frame == anim_atlas->frame_list.size() - 1);

		// 最后一帧停留时间稍长（50%延长）
		int currentInterval = isLastFrame ? interval_ms * 3 / 2 : interval_ms;

		// 计算动画进度百分比 (0.0 - 1.0)
		float progress = static_cast<float>(timer) / currentInterval;

		// 进度超过100%，切换到下一帧
		if (progress >= 1.0f)
		{
			idx_frame = (idx_frame + 1) % anim_atlas->frame_list.size();
			timer = 0;
			progress = 0.0f;
		}

		// 添加帧过渡效果（在最后20%时间内混合两帧）
		if (progress > 0.8f)
		{
			// 计算下一帧索引（循环）
			int nextFrame = (idx_frame + 1) % anim_atlas->frame_list.size();

			// 计算混合因子 (0.0 - 1.0)
			float blendFactor = (progress - 0.8f) / 0.2f;

			// 当前帧透明度逐渐降低
			BYTE currentAlpha = static_cast<BYTE>(255 * (1.0f - blendFactor));
			// 下一帧透明度逐渐增加
			BYTE nextAlpha = static_cast<BYTE>(255 * blendFactor);

			// 绘制当前帧（逐渐透明）
			putimage_alpha(x, y, anim_atlas->frame_list[idx_frame], currentAlpha);

			// 绘制下一帧（逐渐显现）
			putimage_alpha(x, y, anim_atlas->frame_list[nextFrame], nextAlpha);
		}
		else
		{
			// 普通绘制（不透明）
			putimage_alpha(x, y, anim_atlas->frame_list[idx_frame]);
		}
	}

private:
	int timer = 0;//动画计时器
	int idx_frame = 0;//动画帧索引
	int interval_ms = 0;
private:
	Atlas* anim_atlas;
};


class Player
{
	public:
		Player()
		{
			anim_left = new Animation(atlas_player_left,35);
			anim_right = new Animation(atlas_player_right, 35);

		}
		~Player()
		{
			delete anim_left;
			delete anim_right;
		}
		void ProcessEvent(const ExMessage& msg)
		{
			switch (msg.message)
			{
			case WM_KEYDOWN:
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
				break;
			case WM_KEYUP:

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
				break;
			}
		}
			void Move()
			{ 
				int dir_x = is_move_right - is_move_left;
				int dir_y = is_move_down - is_move_up;
				double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);//标准化距离 具体作用在下面的函数体
				if (len_dir != 0)
				{
					double normalized_x = dir_x / len_dir;
					double normalized_y = dir_y / len_dir;
					position.x += (int)(SPEED * normalized_x);
					position.y += (int)(SPEED * normalized_y);
				}//标准化，使得斜向速度仍然为PLAYER_SPEED

				if (position.x < 0) position.x = 0;
				if (position.y < 0) position.y = 0;
				if (position.x + PLAYER_WIDTH > WINDOW_WIDTH) position.x = WINDOW_WIDTH - PLAYER_WIDTH;
				if (position.x + PLAYER_WIDTH > WINDOW_WIDTH) position.x = WINDOW_WIDTH - PLAYER_WIDTH;
			}
			void Draw(int delta)
			{
				static bool facing_left = false;
				int dir_x = is_move_right - is_move_left;

				if (dir_x < 0) facing_left = true;
				else if (dir_x > 0) facing_left = false;

				if (facing_left) anim_left->Play(position.x, position.y, delta);
				else anim_right->Play(position.x,position.y, delta);

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
			const int SPEED = 3;//算是步长了 
		public:
			const int PLAYER_WIDTH = 80;
			const int PLAYER_HEIGHT = 80;
};
class Bullet//子弹，武器
{
	public:
		POINT position = { 0,0 };

	public:
		Bullet() = default;
		~Bullet() = default;

		void Draw() const
		{
			setlinecolor(RGB(255, 255, 255));
			setfillcolor(RGB(255, 255, 255));
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
			anim_left = new Animation(atlas_enemy_left,45);
			anim_right = new Animation(atlas_enmey_right, 45);

			//敌人生成边界
			enum class EmergeEDGE
			{
				Up = 0, Down, Left, Right
			};

			//敌人生成在地图边界处随机位置
			EmergeEDGE edge = (EmergeEDGE)(rand() % 4);
			switch (edge)
			{
			case EmergeEDGE::Up:
				position.x = rand() % WINDOW_WIDTH;
				position.y = -ENEMY_HEIGHT;
				break;
			case EmergeEDGE::Down:
				position.x = rand() % WINDOW_WIDTH;
				position.y = WINDOW_HEIGHT;
				break;
			case EmergeEDGE::Left:
				position.x = -ENEMY_WIDTH;
				position.y = rand() % WINDOW_HEIGHT;
				break;
			case EmergeEDGE::Right:
				position.x = WINDOW_WIDTH;
				position.y = rand() % WINDOW_HEIGHT;
				break;
			default:
				break;
			}
		}
		~Enemy()
		{
			delete anim_left;
			delete anim_right;
		}
		bool CheckBulletHit(const Bullet& bullet)
		{
			//子弹，判断子弹是否在敌人的矩形边界中
			bool is_cover_x = bullet.position.x >= position.x && bullet.position.x <= position.x + ENEMY_WIDTH;
			bool is_cover_y = bullet.position.y >= position.y && bullet.position.y <= position.y + ENEMY_HEIGHT;
			return is_cover_x && is_cover_y;
		}
		bool CheckPlayerHit(const Player& player)//受击碰撞箱
		{
			//取敌人中心为碰撞检测点位
			POINT check_position = { position.x + ENEMY_WIDTH / 2,position.y + ENEMY_HEIGHT / 2 };
			bool is_cover_x = check_position.x >= player.GetPosition().x && check_position.x <= player.GetPosition().x + player.PLAYER_WIDTH;
			bool is_cover_y = check_position.y >= player.GetPosition().y && check_position.y <= player.GetPosition().y+player.PLAYER_HEIGHT;
			return is_cover_x&& is_cover_y;
		}

		void Move(const Player& player)//敌人用来追踪玩家的函数
		{
			//仿照了人物的move
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
			if (dir_x < 0) facing_left = true;
			else facing_left = false;
		}

		void Draw(int delta)
		{
			if (facing_left) anim_left->Play(position.x, position.y, delta);
			else anim_right->Play(position.x, position.y, delta);
		}

		void Hurt()//敌人受伤
		{
			alive = false;
		}

		bool CheckAlive()//是否存活
		{
			return alive;
		}
		
	private:
		const int SPEED = 2;
		const int ENEMY_WIDTH = 80;
		const int ENEMY_HEIGHT = 80;


		Animation* anim_left;
		Animation* anim_right;
		POINT position = { 0,0 };
		bool facing_left = false;
		bool alive = true;
};

//UI
class Button
{
public:
	Button(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
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
	Status status = Status::Idle;
private:
	bool CheckCursorHit(int x, int y)
	{
		return x >= region.left && x <= region.right &&
			y >= region.top && y <= region.bottom;
	}
};
// 开始游戏按钮
class StartGameButton : public Button
{
public:
	StartGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		: Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {
	}

	~StartGameButton() = default;

protected:
	void OnClick() override
	{
		is_game_started = true;

		mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);//播放背景音乐
	}
};

// 退出游戏按钮
class QuitGameButton : public Button
{
public:
	QuitGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		: Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {
	}

	~QuitGameButton() = default;

protected:
	void OnClick() override
	{
		running = false;
	}
};

//生成新敌人
void TryGenerateEnemy(std::vector<Enemy*>& enemy_list)
{
	const int INTERVAL = 100;
	static int counter = 0;
	if ((++counter) % INTERVAL == 0) enemy_list.push_back(new Enemy());
}

//更新子弹
void UpdateBullets(std::vector<Bullet>& bullet_list, const Player& player)
{
	const double RADIUS_SPEED = 0.0045;//径向速度
	const double TANGENT_SPEED = 0.0055;//切向速度
	double radius_interval = 2 * 3.1415 / bullet_list.size();//弧度间隔
	POINT player_position = player.GetPosition();
	double radius = 100 + 25 * sin(GetTickCount() * RADIUS_SPEED);//径向移动
	for (size_t i = 0;i < bullet_list.size();i++)
	{
		double rad = GetTickCount() * TANGENT_SPEED + radius_interval * i;//弧度
		bullet_list[i].position.x = player_position.x + player.PLAYER_WIDTH / 2 + (int)(radius * cos(rad));//R*COSα
		bullet_list[i].position.y = player_position.y + player.PLAYER_HEIGHT / 2 + (int)(radius * sin(rad));
	}
}

//绘制得分
void DrawPlayerScore(int score)
{
	static TCHAR text[64];
	_stprintf_s(text, _T("玩家得分：%d"), score);

	setbkmode(TRANSPARENT);
	settextcolor(RGB(255, 85, 185));
	outtextxy(10, 10, text);
}

int main()
{
	initgraph(1280, 720, 0);

	atlas_player_left = new Atlas(_T("image/Wizard_left_%d.png"), 9);
	atlas_player_right= new Atlas(_T("image/Wizard_right_%d.png"), 9);
	atlas_enemy_left= new Atlas(_T("image/actor1_left_%d.png"), 4);
	atlas_enmey_right= new Atlas(_T("image/actor1_right_%d.png"), 4);//后面记得delete这些指针

	//音乐部分
	mciSendString(_T("open mus/bgm.mp3 alias bgm"), NULL, 0, NULL);//加载
	mciSendString(_T("open mus/hit.mp3 alias hit"), NULL, 0, NULL);
	mciSendString(_T("open mus/fail.mp3 alias fail"), NULL, 0, NULL);

	


	Player player;
	ExMessage msg;
	IMAGE img_background;
	IMAGE img_menu;
	std::vector<Enemy*> enemy_list;
	std::vector<Bullet> bullet_list(3);
	int score = 0;

	RECT region_btn_start_game, region_btn_quit_game;

	// 开始游戏按钮区域
	region_btn_start_game.left = (WINDOW_WIDTH - BUTTON_WIDTH) / 2;
	region_btn_start_game.right = region_btn_start_game.left + BUTTON_WIDTH;
	region_btn_start_game.top = 200;
	region_btn_start_game.bottom = region_btn_start_game.top + BUTTON_HEIGHT;

	// 退出游戏按钮区域
	region_btn_quit_game.left = (WINDOW_WIDTH - BUTTON_WIDTH) / 2;
	region_btn_quit_game.right = region_btn_quit_game.left + BUTTON_WIDTH;
	region_btn_quit_game.top = 300;
	region_btn_quit_game.bottom = region_btn_quit_game.top + BUTTON_HEIGHT;

	StartGameButton btn_start_game = StartGameButton(
		region_btn_start_game,
		_T("image/ui_start_idle.png"),
		_T("image/ui_start_hovered.png"),
		_T("image/ui_start_pushed.png")
	);

	QuitGameButton btn_quit_game = QuitGameButton(
		region_btn_quit_game,
		_T("image/ui_quit_idle.png"),
		_T("image/ui_quit_hovered.png"),
		_T("image/ui_quit_pushed.png")
	);

	loadimage(&img_menu,_T("image/menu.png"));
	loadimage(&img_background, _T("image/background.png"));

	BeginBatchDraw();

	while (running)
	{
		DWORD start_time = GetTickCount();
		while (peekmessage(&msg))
		{
			if (is_game_started)
			{
				player.ProcessEvent(msg);
			}
			else
			{
				btn_start_game.ProcessEvent(msg);
				btn_quit_game.ProcessEvent(msg);
			}
		}

		if (is_game_started)
		{
			player.Move();
			UpdateBullets(bullet_list, player);
			TryGenerateEnemy(enemy_list);
			for (Enemy* enemy : enemy_list)//遍历
				enemy->Move(player);

			//检测敌人的碰撞逻辑
				//和玩家碰撞------------------------------------------------------------------------------------死亡
			for (Enemy* enemy : enemy_list)
			{
				if (enemy->CheckPlayerHit(player))
				{
					static TCHAR text[64];
					_stprintf_s(text, _T("玩家得分：%d"), score);
					MessageBox(GetHWnd(), text, _T("Game Over"), MB_OK);
					mciSendString(_T("play fail from 0"), NULL, 0, NULL);
					running = false;
					break;
				}
			}
			//和子弹碰撞
			for (Enemy* enemy : enemy_list)
			{
				for (const Bullet& bullet : bullet_list)
				{
					if (enemy->CheckBulletHit(bullet))
					{
						mciSendString(_T("play hit from 0"), NULL, 0, NULL);//播放击打音效
						enemy->Hurt();
					}
				}
			}
			//清除生命值为零的敌人
			for (size_t i = 0;i < enemy_list.size();i++)
			{
				Enemy* enemy = enemy_list[i];
				if (!enemy->CheckAlive())
				{
					std::swap(enemy_list[i], enemy_list.back());
					enemy_list.pop_back();
					delete enemy;//切记delete
					score++;//得分这一块
				}
			}
		}
		cleardevice();

		if (is_game_started)
		{
			putimage(0, 0, &img_background);
			player.Draw(1000 / 144);
			for (Enemy* enemy : enemy_list)//遍历
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
	delete atlas_enemy_left;
	delete atlas_enmey_right;
	delete atlas_player_left;
	delete atlas_player_right;

	EndBatchDraw();

	return 0;
}