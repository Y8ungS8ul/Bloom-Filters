#include "ProgramMenu.h"


// ����� ��������� ��������� �������� ������� �������� ����
void MainMenu::AppMenu::setInitText(sf::Text& text, sf::String str, float xpos, float ypos)
{
	text.setFont(font);                 // �����
	text.setFillColor(menu_text_color); // ���� 
	text.setString(str);                // �����
	text.setCharacterSize(size_font);   // ������ ������
	text.setPosition(xpos, ypos);       // ���������� ���������� �������
	text.setOutlineThickness(3);        // ������� ������� ������� ������
	text.setOutlineColor(border_color); // ���� ������� ������� ������
}

// ������������ ������� ���� �� ������ �� ������� �� ������ 
void MainMenu::AppMenu::AlignMenu(int posx, float windowWidth)
{
	float totalWidth = 0;
	

	

	

	for (int i = 0; i < max_menu; i++)
	{
		switch (posx)
		{
		case 0:
			// ������������ �� ������� ���� �� ������������� ���������
			mainMenu[i].setPosition(windowWidth - mainMenu[i].getLocalBounds().width, mainMenu[i].getPosition().y);
			break;
		case 1:
			// ������������ �� ������ ����
			mainMenu[i].setPosition(0, mainMenu[i].getPosition().y);
			break;
		case 2:
			// ������������ �� ������
			totalWidth = mainMenu[i].getLocalBounds().width;
			float centerX = (windowWidth - totalWidth) / 2.0; // ��������� �������� ��� �������������
			mainMenu[i].setPosition(centerX, mainMenu[i].getPosition().y+200);
			//centerX += mainMenu[i].getLocalBounds().width; // ��������� �������� ��� ���������� ��������
			//����� �������� ������ ����, ����� �� ��������� ��������������
			break;
		}
	}
}



// �����������
MainMenu::AppMenu::AppMenu(sf::RenderWindow& window, float menux, float menuy,
	int index, sf::String name[], int sizeFont, int step)
	:mywindow(window), menu_X(menux), menu_Y(menuy), size_font(sizeFont), menu_Step(step)
{
	// �������� ������
	if (!font.loadFromFile("fonts/times.ttf")) exit(32);
	max_menu = index; // ���������� ��������� ���� 
	mainMenu = new sf::Text[max_menu];     // ������������ ������ ������� ����
	// ����������� �������� ����
	for (int i = 0, ypos = menu_Y; i < max_menu; i++, ypos += menu_Step) setInitText(mainMenu[i], name[i], menu_X, ypos);
	mainMenuSelected = 0; // ����� ��������� ��������� ��������� ������ ����
	// ���� ��������� ������ ����
	mainMenu[mainMenuSelected].setFillColor(chose_text_color);
}

// ����������� ������ ���� �����
void MainMenu::AppMenu::MoveUp()
{
	mainMenuSelected--;
	// ������������ ��������� ����� ����
	if (mainMenuSelected >= 0) {
		mainMenu[mainMenuSelected].setFillColor(chose_text_color);
		mainMenu[mainMenuSelected + 1].setFillColor(menu_text_color);
	}
	else
	{
		mainMenu[0].setFillColor(menu_text_color);
		mainMenuSelected = max_menu - 1;
		mainMenu[mainMenuSelected].setFillColor(chose_text_color);
	}
}
// ����������� ������ ���� ����
void MainMenu::AppMenu::MoveDown()
{
	mainMenuSelected++;
	// ������������ ��������� ����� ����
	if (mainMenuSelected < max_menu) {
		mainMenu[mainMenuSelected - 1].setFillColor(menu_text_color);
		mainMenu[mainMenuSelected].setFillColor(chose_text_color);
	}
	else
	{
		mainMenu[max_menu - 1].setFillColor(menu_text_color);
		mainMenuSelected = 0;
		mainMenu[mainMenuSelected].setFillColor(chose_text_color);
	}

}
// ������ �������� ���� � ����������� ����
void MainMenu::AppMenu::draw()
{
	// ���������� ��� ��������� ������������ ��������� ������� ������� ���� 	
	for (int i = 0; i < max_menu; i++) mywindow.draw(mainMenu[i]);
}
// ���������� ����� ��������� ������� ����
void MainMenu::AppMenu::setColorTextMenu(sf::Color menColor, sf::Color ChoColor, sf::Color BordColor)
{
	menu_text_color = menColor;   // ���� ������� ����
	chose_text_color = ChoColor; // ���� ��������� ������ ����
	border_color = BordColor;    // ���� ������� ������� ����

	for (int i = 0; i < max_menu; i++) {
		mainMenu[i].setFillColor(menu_text_color);
		mainMenu[i].setOutlineColor(border_color);
	}

	mainMenu[mainMenuSelected].setFillColor(chose_text_color);
}