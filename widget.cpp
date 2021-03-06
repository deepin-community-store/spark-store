#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QVBoxLayout>
#include <fstream>
#include <QDir>
#include <QProcess>
#include <QJsonDocument>
#include <QFile>
#include <QJsonObject>
#include <QByteArray>
#include <QPixmap>
#include <QtConcurrent> //并发
#include <QSettings>
#include <QIcon>
#include <QWebFrame>
#include <QGraphicsOpacityEffect>
#include <QDesktopServices>
#include <DSettings>
#include <DSettingsOption>
#include <DSettingsDialog>
#include "image_show.h"
#include <DBlurEffectWidget>
#include <QClipboard>
#include <DApplication>
#include <DGuiApplicationHelper>
DWIDGET_USE_NAMESPACE

Widget::Widget(DBlurEffectWidget *parent) :
    DBlurEffectWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    initUI();
    initConfig();
    manager = new QNetworkAccessManager(this);//下载管理

    connect(ui->menu_main,&QPushButton::clicked,[=](){Widget::chooseLeftMenu(0);});
    connect(ui->menu_network,&QPushButton::clicked,[=](){Widget::chooseLeftMenu(1);});
    connect(ui->menu_chat,&QPushButton::clicked,[=](){Widget::chooseLeftMenu(2);});
    connect(ui->menu_music,&QPushButton::clicked,[=](){Widget::chooseLeftMenu(3);});
    connect(ui->menu_video,&QPushButton::clicked,[=](){Widget::chooseLeftMenu(4);});
    connect(ui->menu_photo,&QPushButton::clicked,[=](){Widget::chooseLeftMenu(5);});
    connect(ui->menu_game,&QPushButton::clicked,[=](){Widget::chooseLeftMenu(6);});
    connect(ui->menu_office,&QPushButton::clicked,[=](){Widget::chooseLeftMenu(7);});
    connect(ui->menu_read,&QPushButton::clicked,[=](){Widget::chooseLeftMenu(8);});
    connect(ui->menu_dev,&QPushButton::clicked,[=](){Widget::chooseLeftMenu(9);});
    connect(ui->menu_system,&QPushButton::clicked,[=](){Widget::chooseLeftMenu(10);});
    connect(ui->menu_theme,&QPushButton::clicked,[=](){Widget::chooseLeftMenu(11);});
    connect(ui->menu_other,&QPushButton::clicked,[=](){Widget::chooseLeftMenu(12);});
    connect(ui->menu_download,&QPushButton::clicked,[=](){Widget::chooseLeftMenu(13);});
//    connect((ui->titlebar))


    //搜索事件
    connect(searchEdit,&DSearchEdit::editingFinished,this,[=](){
        QString searchtext=searchEdit->text();
        if(searchtext!=""){
            qDebug()<<searchEdit->text();
            searchApp(searchtext);
        }
        searchEdit->clearEdit();

    });
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [=](DGuiApplicationHelper::ColorType themeType) {
        QColor main_color;
        main_color=DGuiApplicationHelper::instance()->applicationPalette().highlight().color();
        if(themeType==DGuiApplicationHelper::DarkType){
            qDebug()<<"Dark";
            setTheme(true,main_color);
        }else {
            qDebug()<<"White";
            setTheme(false,main_color);
        }
    });

    //计算显示下载速度
    download_speed.setInterval(1000);
    download_speed.start();
    connect(&download_speed,&QTimer::timeout,[=](){
        if(isdownload){
            size1=download_size;
            QString theSpeed;
            double bspeed;
            bspeed=size1-size2;
            if(bspeed<1024){
                theSpeed=QString::number(bspeed)+"B/s";
            }else if (bspeed<(1024*1024)) {
                theSpeed=QString::number(0.01*int(100*(bspeed/1024)))+"KB/s";
            }else if (bspeed<(1024*1024*1024)) {
                theSpeed=QString::number(0.01*int(100*(bspeed/(1024*1024))))+"MB/s";
            }else {
                theSpeed=QString::number(0.01*int(100*(bspeed/(1024*1024*1024))))+"GB/s";
            }
            download_list[nowDownload-1].setSpeed(theSpeed);
            size2=download_size;
        }
    });
}


Widget::~Widget()
{
    delete ui;
    qDebug()<<"exit";
    DApplication::quit();
}
void Widget::initUI()
{
    //ui初始化
    setMaskAlpha(220);
    ui->webfoot->setFixedHeight(0);
    ui->stackedWidget->setCurrentIndex(0);
    ui->listWidget->hide();
    ui->label_setting1->hide();
    ui->pushButton_uninstall->hide();
    ui->line1_widget->setStyleSheet("background-color:#808080");
    ui->icon->setPixmap(QIcon::fromTheme("spark-store").pixmap(36,36));
    ui->titlebar->setFixedHeight(50);



    //初始化分界线
    QGraphicsOpacityEffect *opacityEffect_1=new QGraphicsOpacityEffect;
    opacityEffect_1->setOpacity(0.1);
    ui->line1_widget->setGraphicsEffect(opacityEffect_1);



    //搜索框
    QWidget *w_titlebar=new QWidget;
    QHBoxLayout *ly_titlebar=new QHBoxLayout;
    w_titlebar->setLayout(ly_titlebar);
//    ly_titlebar->addWidget(ui->pushButton_return);
    ly_titlebar->addStretch();
    ly_titlebar->addSpacing(50);
    ly_titlebar->addWidget(searchEdit);
    ly_titlebar->addStretch();
    titlebar=ui->titlebar;
    titlebar->setCustomWidget(w_titlebar);
//    titlebar->setIcon(QIcon::fromTheme("spark-store"));
    titlebar->setTitle("星火应用商店");
    searchEdit->setPlaceholderText("搜索或打开链接");
    searchEdit->setFixedWidth(300);
    titlebar->setSeparatorVisible(false);
//    titlebar->setAutoHideOnFullscreen(true);

    //添加菜单项
    QAction *setting=new QAction("设置");
    QMenu *menu=new QMenu;
    menu->addAction(setting);
    titlebar->setMenu(menu);
    connect(setting,&QAction::triggered,this,&Widget::opensetting);

    //初始化菜单数组
    left_list[0]=ui->menu_main;
    left_list[1]=ui->menu_network;
    left_list[2]=ui->menu_chat;
    left_list[3]=ui->menu_music;
    left_list[4]=ui->menu_video;
    left_list[5]=ui->menu_photo;
    left_list[6]=ui->menu_game;
    left_list[7]=ui->menu_office;
    left_list[8]=ui->menu_read;
    left_list[9]=ui->menu_dev;
    left_list[10]=ui->menu_system;
    left_list[11]=ui->menu_theme;
    left_list[12]=ui->menu_other;
    left_list[13]=ui->menu_download;


    //初始化web加载动画
    QHBoxLayout *m_weblayout=new QHBoxLayout;
    m_weblayout->addWidget(m_loadweb);
    m_weblayout->addWidget(m_loaderror);
    m_loadweb->hide();
    m_loaderror->hide();
    m_loadweb->start();
    m_loadweb->setMaximumSize(50,50);
    m_loadweb->setMinimumSize(50,50);
    m_loadweb->setTextVisible(false);
    m_loaderror->setPixmap(QIcon::fromTheme("dialog-error").pixmap(50,50));
    m_loaderror->setAlignment(Qt::AlignCenter);

    ui->webView->setLayout(m_weblayout);
//    ui->stackedWidget->setLayout(m_weblayout);
    ui->label_show->hide();

}

void Widget::initConfig()
{
    //读取服务器列表并初始化
    std::fstream serverList;
    serverList.open(QDir::homePath().toUtf8()+"/.config/spark-store/server.list",std::ios::in);
    std::string lineTmp;
    if(serverList){
        while (getline(serverList,lineTmp)) {
            ui->comboBox_server->addItem(QString::fromStdString(lineTmp));
        }
    }else {
        ui->comboBox_server->addItem("http://store.jerrywang.top/");
    }



    //读取服务器URL并初始化菜单项的链接
    QSettings readConfig(QDir::homePath()+"/.config/spark-store/config.ini",QSettings::IniFormat);
    if(readConfig.value("server/choose").toString()!=""){
        ui->comboBox_server->setCurrentText(readConfig.value("server/choose").toString());
        serverUrl=readConfig.value("server/choose").toString();
    }else {
        serverUrl="http://store.jerrywang.top/";//默认URL
    }
    configCanSave=true;   //防止出发保存配置信号
    menuUrl[0]=serverUrl + "store/#/";
//    menuUrl[0]="http://127.0.0.1:8000/#/darkprogramming";
    menuUrl[1]=serverUrl + "store/#/network";
    menuUrl[2]=serverUrl + "store/#/relations";
    menuUrl[3]=serverUrl + "store/#/musicandsound";
    menuUrl[4]=serverUrl + "store/#/videos";
    menuUrl[5]=serverUrl + "store/#/photos";
    menuUrl[6]=serverUrl + "store/#/games";
    menuUrl[7]=serverUrl + "store/#/office";
    menuUrl[8]=serverUrl + "store/#/reading";
    menuUrl[9]=serverUrl + "store/#/programming";
    menuUrl[10]=serverUrl + "store/#/tools";
    menuUrl[11]=serverUrl + "store/#/themes";
    menuUrl[12]=serverUrl + "store/#/others";


    //web控件初始化
    ui->webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);   //用来激活接受linkClicked信号
    ui->webView->page()->settings()->setAttribute(QWebSettings::JavascriptEnabled,true);
    ui->webfoot->hide();

    //初始化首页
    ui->webView->setUrl(menuUrl[0]);
    chooseLeftMenu(0);

    //给下载列表赋值到数组，方便调用
    for (int i =0; i<LIST_MAX;i++){
        download_list[i].num=i;
    }

    //初始化apt源显示
    QFile aptserver("/etc/apt/sources.list.d/sparkstore.list");
    aptserver.open(QIODevice::ReadOnly);
    if(aptserver.isOpen()){
        ui->label_aptserver->setText(aptserver.readAll());
    }else {
        ui->label_aptserver->setText("不存在");
    }
    aptserver.close();

    //新建临时文件夹
    QDir dir("/tmp");
    dir.mkdir("spark-store");
}
void Widget::setTheme(bool isDark,QColor color)
{
    if(isDark){
        //黑色模式
        themeIsDark=true;
        ui->webView->setStyleSheet("background-color:#282828");
        ui->btn_openDir->setStyleSheet("color:#8B91A1;background-color:#2E2F30;border:0px");
        ui->webfoot->setStyleSheet("background-color:#252525");
        ui->label->setStyleSheet("background-color:#252525");
        ui->scrollArea->setStyleSheet("#scrollArea{background-color:#252525}");
        ui->label_show->setStyleSheet("background-color:#252525");
        ui->pushButton_return->setIcon(QIcon(":/icons/icons/category_active_dark.svg"));
        //菜单图标


    }else {
        //亮色模式
        themeIsDark=false;
        ui->webView->setStyleSheet("background-color:#FFFFFF");
        ui->webfoot->setStyleSheet("background-color:#FFFFFF");
        ui->btn_openDir->setStyleSheet("color:#505050;background-color:#FBFBFB;border:0px");
        ui->label->setStyleSheet("background-color:#FFFFFF");
        ui->scrollArea->setStyleSheet("#scrollArea{background-color:#F8F8F8}");
        ui->label_show->setStyleSheet("background-color:#F8F8F8");
        ui->pushButton_return->setIcon(QIcon(":/icons/icons/category_active.svg"));

    }
    main_color=color;

    updateUI();
    if(ui->stackedWidget->currentIndex()==0){
        chooseLeftMenu(nowMenu);
    }

}

DTitlebar* Widget::getTitlebar()
{
    return ui->titlebar;
}

void Widget::on_webView_loadStarted()
{
    m_loadweb->setValue(0);
    m_loadweb->show();
    m_loaderror->hide();
    ui->label_show->hide();

    //分析出服务器中的分类名称
    QUrl arg1=ui->webView->page()->mainFrame()->requestedUrl().toString();
    QStringList url_=arg1.path().split("/");
    if(url_.size()>3){
        type_name=url_[2];
    }
    //如果是app.json就打开详情页
    if(arg1.path().right(8)=="app.json"){
        load.cancel();//打开并发加载线程前关闭正在执行的线程

        ui->label_more->setText("");//清空详情介绍
        ui->label_info->setText("");
        ui->label_appname->setText("");
        ui->pushButton_download->setEnabled(false);
        ui->stackedWidget->setCurrentIndex(2);
        load.cancel();//打开并发加载线程前关闭正在执行的线程
        load = QtConcurrent::run([=](){
            loadappinfo(arg1);
        });
    }
}
void Widget::updateUI()
{
    if(themeIsDark){
        left_list[0]->setIcon(QIcon(":/icons/icons/homepage_dark.svg"));
        left_list[1]->setIcon(QIcon(":/icons/icons/category_network_dark.svg"));
        left_list[2]->setIcon(QIcon(":/icons/icons/category_chat_dark.svg"));
        left_list[3]->setIcon(QIcon(":/icons/icons/category_music_dark.svg"));
        left_list[4]->setIcon(QIcon(":/icons/icons/category_video_dark.svg"));
        left_list[5]->setIcon(QIcon(":/icons/icons/category_graphic_dark.svg"));
        left_list[6]->setIcon(QIcon(":/icons/icons/category_game_dark.svg"));
        left_list[7]->setIcon(QIcon(":/icons/icons/category_office_dark.svg"));
        left_list[8]->setIcon(QIcon(":/icons/icons/category_reading_dark.svg"));
        left_list[9]->setIcon(QIcon(":/icons/icons/category_develop_dark.svg"));
        left_list[10]->setIcon(QIcon(":/icons/icons/category_system_dark.svg"));
        left_list[11]->setIcon(QIcon(":/icons/icons/theme-symbolic_dark.svg"));
        left_list[12]->setIcon(QIcon(":/icons/icons/category_others_dark.svg"));
        left_list[13]->setIcon(QIcon(":/icons/icons/downloads-symbolic_dark.svg"));
    }else {
        left_list[0]->setIcon(QIcon(":/icons/icons/homepage.svg"));
        left_list[1]->setIcon(QIcon(":/icons/icons/category_network.svg"));
        left_list[2]->setIcon(QIcon(":/icons/icons/category_chat.svg"));
        left_list[3]->setIcon(QIcon(":/icons/icons/category_music.svg"));
        left_list[4]->setIcon(QIcon(":/icons/icons/category_video.svg"));
        left_list[5]->setIcon(QIcon(":/icons/icons/category_graphic.svg"));
        left_list[6]->setIcon(QIcon(":/icons/icons/category_game.svg"));
        left_list[7]->setIcon(QIcon(":/icons/icons/category_office.svg"));
        left_list[8]->setIcon(QIcon(":/icons/icons/category_reading.svg"));
        left_list[9]->setIcon(QIcon(":/icons/icons/category_develop.svg"));
        left_list[10]->setIcon(QIcon(":/icons/icons/category_system.svg"));
        left_list[11]->setIcon(QIcon(":/icons/icons/theme-symbolic.svg"));
        left_list[12]->setIcon(QIcon(":/icons/icons/category_others.svg"));
        left_list[13]->setIcon(QIcon(":/icons/icons/downloads-symbolic.svg"));
    }
    for (int i=0;i<14;i++) {
        left_list[i]->setFont(QFont("",11));
        left_list[i]->setFixedHeight(38);
        if(themeIsDark){
            left_list[i]->setStyleSheet("color:#FFFFFF;border:0px");
        }else {
            left_list[i]->setStyleSheet("color:#252525;border:0px");
        }
    }
    left_list[nowMenu]->setStyleSheet("color:#FFFFFF;background-color:"+main_color.name()+";border-radius:8;border:0px");
    switch (nowMenu) {
    case 0:
        left_list[0]->setIcon(QIcon(":/icons/icons/homepage_dark.svg"));
        break;
    case 1:
        left_list[1]->setIcon(QIcon(":/icons/icons/category_network_dark.svg"));
        break;
    case 2:
        left_list[2]->setIcon(QIcon(":/icons/icons/category_chat_dark.svg"));
        break;
    case 3:
        left_list[3]->setIcon(QIcon(":/icons/icons/category_music_dark.svg"));
        break;
    case 4:
        left_list[4]->setIcon(QIcon(":/icons/icons/category_video_dark.svg"));
        break;
    case 5:
        left_list[5]->setIcon(QIcon(":/icons/icons/category_graphic_dark.svg"));
        break;
    case 6:
        left_list[6]->setIcon(QIcon(":/icons/icons/category_game_dark.svg"));
        break;
    case 7:
        left_list[7]->setIcon(QIcon(":/icons/icons/category_office_dark.svg"));
        break;
    case 8:
        left_list[8]->setIcon(QIcon(":/icons/icons/category_reading_dark.svg"));
        break;
    case 9:
        left_list[9]->setIcon(QIcon(":/icons/icons/category_develop_dark.svg"));
        break;
    case 10:
        left_list[10]->setIcon(QIcon(":/icons/icons/category_system_dark.svg"));
        break;
    case 11:
        left_list[11]->setIcon(QIcon(":/icons/icons/theme-symbolic_dark.svg"));
        break;
    case 12:
        left_list[12]->setIcon(QIcon(":/icons/icons/category_others_dark.svg"));
        break;
    case 13:
        left_list[13]->setIcon(QIcon(":/icons/icons/downloads-symbolic_dark.svg"));
        break;
    }
}
//菜单切换逻辑

void Widget::chooseLeftMenu(int index)
{

    nowMenu=index;
//    setfoot();
//    updatefoot();

    updateUI();
    left_list[index]->setStyleSheet("color:#FFFFFF;background-color:"+main_color.name()+";border-radius:8;border:0px");
    if(index<=12){
        if(themeIsDark){
            QString darkurl=menuUrl[index].toString();
            QStringList tmp=darkurl.split("/");
            darkurl.clear();
            for (int i=0;i<tmp.size()-1;i++) {
                darkurl+=tmp[i]+"/";
            }
            darkurl+="dark"+tmp[tmp.size()-1];
            ui->webView->setUrl(darkurl);
            qDebug()<<darkurl;
        }else {
            ui->webView->setUrl(menuUrl[index]);
        }

        ui->stackedWidget->setCurrentIndex(0);
    }else if (index==13) {
        ui->stackedWidget->setCurrentIndex(1);
    }

}

void Widget::setfoot(int h)
{
    foot=h;
}

void Widget::updatefoot()
{
    int allh=ui->stackedWidget->height();
    ui->webfoot->setFixedHeight(allh-foot);
}


void Widget::loadappinfo(QUrl arg1)
{

    if(arg1.isEmpty()){
        return;
    }

    //先隐藏详情页负责显示截图的label
    ui->screen_0->hide();
    ui->screen_1->hide();
    ui->screen_2->hide();
    ui->screen_3->hide();
    ui->screen_4->hide();

    //置UI状态
    ui->pushButton_uninstall->hide();
    ui->label_show->setText("正在加载，请稍候");
    ui->label_show->show();
    ui->pushButton_website->hide();

    QProcess get_json;
    QDir dir("/tmp");
    dir.mkdir("spark-store");
    QDir::setCurrent("/tmp/spark-store");

    get_json.start("curl -o app.json "+arg1.toString());
    get_json.waitForFinished();
    QFile app_json("app.json");
    if(app_json.open(QIODevice::ReadOnly)){
        //        //成功得到json文件
        QByteArray json_array=app_json.readAll();
        //将路径转化为相应源的下载路径
        urladdress=arg1.toString().left(arg1.toString().length()-8);
        QStringList downloadurl=urladdress.split("/");
        urladdress=ui->comboBox_server->currentText();
        QString deburl=urladdress;
        deburl=deburl.left(urladdress.length()-1);
        urladdress="http://img.shenmo.tech:38324/";//使用图片专用服务器请保留这行，删除后将使用源服务器
        urladdress=urladdress.left(urladdress.length()-1);

        for (int i=3;i<downloadurl.size();i++) {
            urladdress+="/"+downloadurl[i];
            deburl+="/"+downloadurl[i];
        }
        //路径转化完成
        QJsonObject json= QJsonDocument::fromJson(json_array).object();
        appName = json["Name"].toString();
        url=deburl + json["Filename"].toString();
        qDebug()<<url;
        ui->label_appname->setText(appName);
        system("rm -r *.png");
        ui->label_show->show();
        //软件信息加载
        QString info;
        info="包名： "+json["Pkgname"].toString()+"\n";
        info+="版本号： "+json["Version"].toString()+"\n";
        if(json["Author"].toString()!="" && json["Author"].toString()!=" "){
            info+="作者： "+json["Author"].toString()+"\n";
        }

        if(json["Website"].toString()!="" && json["Website"].toString()!=" "){
            info+="官网： "+json["Website"].toString()+"\n";
            ui->pushButton_website->show();
            appweb=json["Website"].toString();
        }
        info+="投稿者： "+json["Contributor"].toString()+"\n";
        info+="更新时间： "+json["Update"].toString()+"\n";
        info+="大小： "+json["Size"].toString()+"\n";
        ui->label_info->setText(info);
        ui->label_more->setText(json["More"].toString());
        QProcess isInstall;
        pkgName=json["Pkgname"].toString();
        isInstall.start("dpkg -s "+json["Pkgname"].toString());
        isInstall.waitForFinished();
        int error=QString::fromStdString(isInstall.readAllStandardError().toStdString()).length();
        if(error==0){
            ui->pushButton_download->setText("重新安装");
            ui->pushButton_uninstall->show();

        }else {
            ui->pushButton_download->setText("安装");
        }
        //图标加载
        get_json.start("curl -o icon.png "+urladdress+"icon.png");
        get_json.waitForFinished();
        QPixmap appicon(QString::fromUtf8(TMP_PATH)+"/icon.png");
        ui->label_appicon->setPixmap(appicon);
        ui->pushButton_download->setEnabled(true);
        //截图展示加载

        image_show *label_screen[5];
        label_screen[0]=ui->screen_0;
        label_screen[1]=ui->screen_1;
        label_screen[2]=ui->screen_2;
        label_screen[3]=ui->screen_3;
        label_screen[4]=ui->screen_4;
        for (int i=0;i<5;i++) {
            get_json.start("curl -o screen_"+QString::number(i+1)+".png "+urladdress+"screen_"+QString::number(i+1)+".png");
            get_json.waitForFinished();
            if(screen[i].load("screen_"+QString::number(i+1)+".png")){
                label_screen[i]->setImage(screen[i]);
                label_screen[i]->show();
                switch(i){ //故意为之，为了清除多余截图
                case 0:
                    label_screen[1]->hide();
                case 1:
                    label_screen[2]->hide();
                case 2:
                    label_screen[3]->hide();
                case 3:
                    label_screen[4]->hide();
                }
            }else{
                QFile::remove("screen_"+QString::number(i+1)+".png");
                break;
            }
        }
        ui->label_show->setText("");
        ui->label_show->hide();

    }

}




void Widget::on_pushButton_download_clicked()
{
    chooseLeftMenu(13);
    allDownload+=1;
    QFileInfo info(url.path());
    QString fileName(info.fileName());  //获取文件名
    download_list[allDownload-1].pkgName=pkgName;
    if(fileName.isEmpty())
    {
        system("notify-send 获取失败 --icon=spark-store");
        return;
    }
    download_list[allDownload-1].setParent(ui->listWidget);
    QListWidgetItem *item=new QListWidgetItem(ui->listWidget);
    item->setSizeHint(download_list[allDownload-1].size());
    item->setFlags(item->flags() & ~Qt::ItemIsEnabled & ~Qt::ItemIsSelectable);
    ui->listWidget->setItemWidget(item,&download_list[allDownload-1]);
    urList.append(url);
    download_list[allDownload-1].setName(appName);
    download_list[allDownload-1].setFileName(fileName);
    QPixmap icon;
    icon.load("icon.png");
    system("cp icon.png icon_"+QString::number(allDownload-1).toUtf8()+".png");
    download_list[allDownload-1].seticon(icon);
    if(!isBusy){
        file = new QFile(fileName);
        if(!file->open(QIODevice::WriteOnly)){
            delete file;
            file = nullptr;
            return ;
        }
        nowDownload+=1;
        startRequest(urList.at(nowDownload-1)); //进行链接请求
    }
    if(ui->pushButton_download->text()=="重新安装"){
        download_list[allDownload-1].reinstall=true;
    }
}

void Widget::startRequest(QUrl url)
{
    ui->listWidget->show();
    ui->label->hide();
    isBusy=true;
    isdownload=true;
    download_list[allDownload-1].free=false;
    reply = manager->get(QNetworkRequest(url));
    connect(reply,SIGNAL(finished()),this,SLOT(httpFinished()));
    connect(reply,SIGNAL(readyRead()),this,SLOT(httpReadyRead()));
    connect(reply,SIGNAL(downloadProgress(qint64,qint64)),this,SLOT(updateDataReadProgress(qint64,qint64)));

}

void Widget::searchApp(QString text)
{
    if(text.left(6)=="spk://"){
        openUrl(text);
    }else {
        system("notify-send 目前仅支持商店专用链接的打开，搜索功能正在开发，请期待以后的版本！ --icon=spark-store");
//        ui->webView->setUrl(QUrl("http://www.baidu.com/s?wd="+text));
//        ui->stackedWidget->setCurrentIndex(0);

    }

}


void Widget::httpReadyRead()
{
    if(file)
    {
        file->write(reply->readAll());
    }
}
void Widget::updateDataReadProgress(qint64 bytesRead, qint64 totalBytes)
{
    download_list[nowDownload-1].setMax(10000); //最大值
    download_list[nowDownload-1].setValue((bytesRead*10000)/totalBytes); //当前值
    download_size=bytesRead;
    if(download_list[nowDownload-1].close){ //随时检测下载是否被取消
        download_list[nowDownload-1].closeDownload();
        httpFinished();
    }
}

void Widget::httpFinished() //完成下载
{

    file->flush();
    file->close();
    reply->deleteLater();
    reply = nullptr;
    delete file;
    file = nullptr;
    isdownload=false;
    isBusy=false;
    download_list[nowDownload-1].readyInstall();
    download_list[nowDownload-1].free=true;
    if(nowDownload<allDownload){ //如果有排队则下载下一个
        nowDownload+=1;
        while (download_list[nowDownload-1].close) {
            nowDownload+=1;
        }
        QString fileName=download_list[nowDownload-1].getName();
        file = new QFile(fileName);
        if(!file->open(QIODevice::WriteOnly))
        {
            delete file;
            file = nullptr;
            return ;
        }
        startRequest(urList.at(nowDownload-1));
    }
}



void Widget::on_pushButton_return_clicked()
{
//    ui->stackedWidget->setCurrentIndex(0);
//    if(nowMenu==13){
//        chooseLeftMenu(13);
//        return;
//    }
    chooseLeftMenu(nowMenu);
//    if(themeIsDark){
//        QString darkurl=menuUrl[nowMenu].toString();
//        QStringList tmp=darkurl.split("/");
//        darkurl.clear();
//        for (int i=0;i<tmp.size()-1;i++) {
//            darkurl+=tmp[i]+"/";
//        }
//        darkurl+="dark"+tmp[tmp.size()-1];
//        ui->webView->setUrl(darkurl);
//        qDebug()<<darkurl;
//    }else {
//        ui->webView->setUrl(menuUrl[nowMenu]);
//    }
}

void Widget::on_comboBox_server_currentIndexChanged(const QString &arg1)
{
    if(configCanSave){
        ui->label_setting1->show();
        QSettings *setConfig=new QSettings(QDir::homePath()+"/.config/spark-store/config.ini",QSettings::IniFormat);
        setConfig->setValue("server/choose",arg1);
    }
}
void Widget::on_pushButton_updateServer_clicked()
{
    QtConcurrent::run([=](){
        ui->pushButton_updateServer->setEnabled(false);
        ui->comboBox_server->clear();
        QFile::remove(QDir::homePath().toUtf8()+"/.config/spark-store/server.list");
        system("curl -o "+QDir::homePath().toUtf8()+"/.config/spark-store/server.list http://dcstore.shenmo.tech/store/server.list");
        std::fstream server;
        server.open(QDir::homePath().toUtf8()+"/.config/spark-store/server.list",std::ios::in);
        std::string lineTmp;
        if(server){
            while (getline(server,lineTmp)) {
                ui->comboBox_server->addItem(QString::fromStdString(lineTmp));
            }
        }else {
            ui->comboBox_server->addItem("http://store.jerrywang.top/");
        }
        ui->pushButton_updateServer->setEnabled(true);
        ui->comboBox_server->setCurrentIndex(0);
    });
}

void Widget::on_pushButton_updateApt_clicked()
{
    QtConcurrent::run([=](){
       ui->pushButton_updateApt->setEnabled(false);
       ui->label_aptserver->setText("请稍等，正在更新");
       std::fstream sourcesList;
       QDir tmpdir("/tmp");
       tmpdir.mkpath("spark-store");
       sourcesList.open(QString::fromUtf8(TMP_PATH).toStdString()+"/sparkstore.list",std::ios::out);
       if(sourcesList){
           sourcesList<<"deb [by-hash=force] ";
           sourcesList<<QString::fromUtf8(ui->comboBox_server->currentText().toUtf8()).toStdString();
           sourcesList<<" /";
           std::fstream update;
           update.open(QString::fromUtf8(TMP_PATH).toStdString()+"/update.sh",std::ios::out);
           update<<"#!/bin/sh\n";
           update<<"mv "+QString::fromUtf8(TMP_PATH).toStdString()+"/sparkstore.list /etc/apt/sources.list.d/sparkstore.list && apt update";
           update.close();
           system("chmod +x "+QString::fromUtf8(TMP_PATH).toUtf8()+"/update.sh");
           QProcess runupdate;
           runupdate.start("pkexec "+QString::fromUtf8(TMP_PATH)+"/update.sh");
           runupdate.waitForFinished();
           QString error=QString::fromStdString(runupdate.readAllStandardError().toStdString());
           QStringList everyError=error.split("\n");
           bool haveError=false;
           for (int i=0;i<everyError.size();i++) {
               if(everyError[i].left(2)=="E:"){
                   haveError=true;
               }
           }
           if(!haveError){
               ui->label_aptserver->setText("deb [by-hash=force] "+ui->comboBox_server->currentText().toUtf8()+" /");
           }else {
               ui->label_aptserver->setText("更新中发生错误，请在终端使用apt update来查看错误原因");
           }
       }else {
           ui->label_aptserver->setText("服务器错误");
       }

       ui->pushButton_updateApt->setEnabled(true);
    });
}

void Widget::on_pushButton_uninstall_clicked()
{
    QtConcurrent::run([=](){
        ui->pushButton_download->setEnabled(false);
        ui->pushButton_uninstall->setEnabled(false);
        QProcess uninstall;
        uninstall.start("pkexec apt purge -y "+pkgName);
        uninstall.waitForFinished();
        ui->pushButton_download->setEnabled(true);
        ui->pushButton_download->setText("安装");
        ui->pushButton_uninstall->hide();
        ui->pushButton_uninstall->setEnabled(true);
        updatesEnabled();
        system("notify-send 卸载完成 --icon=spark-store");
    });
}

void Widget::on_pushButton_clear_clicked()//清空临时缓存目录
{
    QtConcurrent::run([=](){
        ui->pushButton_clear->setEnabled(false);
        QDir tmpdir("/tmp/spark-store");
        tmpdir.setFilter(QDir::Files);
        int quantity=int(tmpdir.count());
        for (int i=0;i<quantity;i++) {
            tmpdir.remove(tmpdir[i]);
        }
        system("notify-send 已清除所有临时缓存 --icon=spark-store");
        ui->pushButton_clear->setEnabled(true);
        Widget::opensetting();
    });
}

quint64 Widget::dirFileSize(const QString &path)
{
    QDir dir(path);
    quint64 size = 0;
    //dir.entryInfoList(QDir::Files)返回文件信息
    foreach(QFileInfo fileInfo, dir.entryInfoList(QDir::Files))
    {
        //计算文件大小
        size += quint64(fileInfo.size());
    }
    //dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot)返回所有子目录，并进行过滤
    foreach(QString subDir, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        //若存在子目录，则递归调用dirFileSize()函数
        size += dirFileSize(path + QDir::separator() + subDir);
    }
    return size;
}

void Widget::opensetting()
{
    //防止下载时文件被删除
    if(isdownload){
        ui->pushButton_clear->setEnabled(false);
    }else {
        ui->pushButton_clear->setEnabled(true);
    }
    //显示缓存占用空间
    quint64 tmp_size=dirFileSize(QString::fromUtf8(TMP_PATH));
    QString tmp_size_str;
    if(tmp_size<1024){
        tmp_size_str=QString::number(tmp_size)+"B";
    }else if (tmp_size<(1024*1024)) {
        tmp_size_str=QString::number(0.01*int(100*(tmp_size/1024)))+"KB";
    }else if (tmp_size<(1024*1024*1024)) {
        tmp_size_str=QString::number(0.01*int(100*(tmp_size/(1024*1024))))+"MB";
    }else {
        tmp_size_str=QString::number(0.01*int(100*(tmp_size/(1024*1024*1024))))+"GB";
    }
    ui->tmp_size_ui->setText(tmp_size_str);
    ui->stackedWidget->setCurrentIndex(3);
}

void Widget::openUrl(QUrl u)
{
    QString app=serverUrl + "store"+u.path()+"/app.json";
    ui->webView->setUrl(app);
}



void Widget::on_pushButton_website_clicked()
{
    QDesktopServices::openUrl(QUrl(appweb));
}


void Widget::on_webView_loadFinished(bool arg1)
{
    if(arg1){
         m_loadweb->hide();
    }else {
        m_loadweb->hide();
        m_loaderror->show();
    }

}

void Widget::on_webView_loadProgress(int progress)
{
    m_loadweb->setValue(progress);
    if(progress>=90){
        m_loadweb->hide();
    }
}

void Widget::on_pushButton_clicked()
{
    QString share_url;
    share_url="spk://store/"+type_name+"/"+pkgName;
    qDebug()<<"Share"<<share_url;
    QClipboard *clipboard=QApplication::clipboard();
    system("notify-send 链接已经复制到剪贴板 --icon=spark-store");
    clipboard->setText(share_url);
}

void Widget::on_btn_openDir_clicked()
{

    QDesktopServices::openUrl(QUrl("file:///tmp/spark-store", QUrl::TolerantMode));
}

void Widget::on_stackedWidget_currentChanged(int arg1)
{
    qDebug()<<arg1;
    if(arg1==0 || arg1==1){
        ui->pushButton_return->setEnabled(false);
    }else {
        ui->pushButton_return->setEnabled(true);
    }
}
