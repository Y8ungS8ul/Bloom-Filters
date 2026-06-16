#include "../ui/ProgramMenu.h"

/**
 * Инициализация текстового пункта меню
 * Устанавливает шрифт, цвет, размер, позицию и обводку для текстового объекта
 * 
 * @param text - ссылка на объект sf::Text, который нужно настроить
 * @param str - текст пункта меню (например, "Играть", "Выход")
 * @param xpos - координата X расположения пункта
 * @param ypos - координата Y расположения пункта
 */
void MainMenu::AppMenu::setInitText(sf::Text& text, sf::String str, float xpos, float ypos){
    text.setFont(font);                 // устанавливаем шрифт
    text.setFillColor(menu_text_color); // устанавливаем цвет текста
    text.setString(str);                // устанавливаем текст пункта меню
    text.setCharacterSize(size_font);   // устанавливаем размер шрифта
    text.setPosition(xpos, ypos);       // устанавливаем позицию текстового объекта
    text.setOutlineThickness(3);        // устанавливаем толщину обводки текста
    text.setOutlineColor(border_color); // устанавливаем цвет обводки текста
}

/**
 * Конструктор класса AppMenu
 * Создаёт массив текстовых пунктов меню и инициализирует их
 * 
 * @param window - ссылка на графическое окно SFML (для отрисовки)
 * @param menux - начальная координата X для первого пункта меню
 * @param menuy - начальная координата Y для первого пункта меню
 * @param index - количество пунктов в меню
 * @param name - массив строк с названиями пунктов меню
 * @param sizeFont - размер шрифта для пунктов меню (по умолчанию 60)
 * @param step - расстояние между пунктами меню по вертикали (по умолчанию 50)
 */
MainMenu::AppMenu::AppMenu(sf::RenderWindow& window, float menux, float menuy,
    int index, sf::String name[], int sizeFont, int step)
    : mywindow(window), menu_X(menux), menu_Y(menuy), size_font(sizeFont), menu_Step(step) {
    // Загружаем шрифт из файла, при ошибке завершаем программу с кодом 32
    if (!font.loadFromFile("fonts/times.ttf")) exit(32);
    
    max_menu = index; // Количество пунктов меню
    mainMenu = new sf::Text[max_menu]; // Выделение памяти под массив текстовых объектов
    
    // Создаём и инициализируем каждый пункт меню в цикле
    // ypos увеличивается на menu_Step после каждого пункта
    for (int i = 0, ypos = menu_Y; i < max_menu; i++, ypos += menu_Step) 
        setInitText(mainMenu[i], name[i], menu_X, ypos);

    mainMenuSelected = 0; // первый пункт меню выбран по умолчанию
    mainMenu[mainMenuSelected].setFillColor(chose_text_color); // выделяем первый пункт цветом
}

/**
 * Выравнивание меню по горизонтали
 * Изменяет координату X каждого пункта меню в зависимости от выбранного режима
 * 
 * @param posx - режим выравнивания:
 *               0 - правый край (меню прижато к правой стороне)
 *               1 - левый край (меню прижато к левой стороне)
 *               2 - центр (меню отцентрировано по горизонтали)
 * @param windowWidth - ширина окна (нужна для вычисления позиций)
 */
void MainMenu::AppMenu::AlignMenu(int posx, float windowWidth)
{
    for (int i = 0; i < max_menu; i++)
    {
        switch (posx)
        {
        case 0: // Выравнивание по правому краю
            mainMenu[i].setPosition(windowWidth - mainMenu[i].getLocalBounds().width, 
                                      mainMenu[i].getPosition().y);
            break;
            
        case 1: // Выравнивание по левому краю
            mainMenu[i].setPosition(0, mainMenu[i].getPosition().y);
            break;
            
        case 2: // Выравнивание по центру
            {
                // Получаем ширину текущего текста
                float textWidth = mainMenu[i].getLocalBounds().width;
                // Вычисляем координату X для центрирования
                float centerX = (windowWidth - textWidth) / 2.0f;
                mainMenu[i].setPosition(centerX, mainMenu[i].getPosition().y);
            }
            break;
        }
    }
}

/**
 * Перемещение выделения вверх по меню
 * Уменьшает индекс выбранного пункта на 1.
 * При достижении первого пункта переходит на последний (зацикливание)
 */
void MainMenu::AppMenu::MoveUp()
{
    mainMenuSelected--;
    
    // Если новый индекс в пределах массива (не вышел за границу)
    if (mainMenuSelected >= 0) {
        // Новый выбранный пункт окрашиваем в цвет выделения
        mainMenu[mainMenuSelected].setFillColor(chose_text_color);
        // Предыдущий выбранный пункт возвращаем к обычному цвету
        mainMenu[mainMenuSelected + 1].setFillColor(menu_text_color);
    }
    else
    {
        // Если вышли за верхнюю границу (стало -1)
        // Возвращаем первый пункт к обычному цвету
        mainMenu[0].setFillColor(menu_text_color);
        // Переходим на последний пункт меню
        mainMenuSelected = max_menu - 1;
        // Выделяем последний пункт
        mainMenu[mainMenuSelected].setFillColor(chose_text_color);
    }
}

/**
 * Перемещение выделения вниз по меню
 * Увеличивает индекс выбранного пункта на 1.
 * При достижении последнего пункта переходит на первый (зацикливание)
 */
void MainMenu::AppMenu::MoveDown()
{
    mainMenuSelected++;
    
    // Если новый индекс в пределах массива
    if (mainMenuSelected < max_menu) {
        // Предыдущий пункт возвращаем к обычному цвету
        mainMenu[mainMenuSelected - 1].setFillColor(menu_text_color);
        // Новый пункт окрашиваем в цвет выделения
        mainMenu[mainMenuSelected].setFillColor(chose_text_color);
    }
    else
    {
        // Если вышли за нижнюю границу (стало max_menu)
        // Возвращаем последний пункт к обычному цвету
        mainMenu[max_menu - 1].setFillColor(menu_text_color);
        // Переходим на первый пункт меню
        mainMenuSelected = 0;
        // Выделяем первый пункт
        mainMenu[mainMenuSelected].setFillColor(chose_text_color);
    }
}

/**
 * Отрисовка меню в графическом окне
 * Проходит по всем пунктам меню и отрисовывает их в окне mywindow
 */
void MainMenu::AppMenu::draw()
{
    // Выводим все пункты меню в нужное окно
    for (int i = 0; i < max_menu; i++) 
        mywindow.draw(mainMenu[i]);
}

/**
 * Установка цветовой схемы меню
 * Изменяет цвета всех пунктов меню и обводки
 * 
 * @param menColor - цвет обычных (невыделенных) пунктов меню
 * @param ChoColor - цвет выделенного (активного) пункта меню
 * @param BordColor - цвет обводки текста для всех пунктов
 */
void MainMenu::AppMenu::setColorTextMenu(sf::Color menColor, sf::Color ChoColor, sf::Color BordColor)
{
    menu_text_color = menColor;      // сохраняем цвет обычного меню
    chose_text_color = ChoColor;     // сохраняем цвет выбранного пункта
    border_color = BordColor;        // сохраняем цвет обводки

    // Применяем цвета ко всем пунктам меню
    for (int i = 0; i < max_menu; i++) {
        mainMenu[i].setFillColor(menu_text_color);    // устанавливаем цвет текста
        mainMenu[i].setOutlineColor(border_color);    // устанавливаем цвет обводки
    }

    // Отдельно выделяем текущий выбранный пункт
    mainMenu[mainMenuSelected].setFillColor(chose_text_color);
}