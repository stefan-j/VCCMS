#include "userinterfacecreator.h"
#include "ui_userinterfacecreator.h"
#include <QDebug>

UserInterfaceCreator::UserInterfaceCreator(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UserInterfaceCreator){

    ui->setupUi(this);
    // Set the number of columns in the tree
    ui->treeWidget->setColumnCount(2);

    // Set the headers
    ui->treeWidget->setHeaderLabels(QStringList() << "Index" << "Directory Name");
    connect(ui->pushButton_removedirectory,SIGNAL(clicked()),this,SLOT(OnDeleteIt()));

    // populate list with nodes
    load_interface("VirtualConcierge/nodes.pvc");

    // clear the directory list
    m_directories.clear();
}

UserInterfaceCreator::~UserInterfaceCreator(){
    delete ui;
}

void UserInterfaceCreator::on_pushButton_adddirectory_clicked(){
    AddAfterCurrentIndex();
}

void UserInterfaceCreator::addTreeRoot(QString name, QString description){

    // QTreeWidgetItem(QTreeWidget * parent, int type = Type)
    QTreeWidgetItem *treeItem = new QTreeWidgetItem(ui->treeWidget);

    // QTreeWidgetItem::setText(int column, const QString & text)
    treeItem->setText(0, name);
    treeItem->setText(1, description);
}

void UserInterfaceCreator::addTreeChild(QTreeWidgetItem *parent,
                  QString name, QString description){

    // QTreeWidgetItem(QTreeWidget * parent, int type = Type)
    QTreeWidgetItem *treeItem = new QTreeWidgetItem();

    // QTreeWidgetItem::setText(int column, const QString & text)
    treeItem->setText(0, name);
    treeItem->setText(1, description);

    // QTreeWidgetItem::addChild(QTreeWidgetItem * child)
    parent->addChild(treeItem);
}

void UserInterfaceCreator::OnDeleteIt(){
    // delete current selected item
        int removed = ui->treeWidget->currentIndex().row();
    QTreeWidgetItem *item = ui->treeWidget->currentItem();
    if(!item) return;
    delete item;
    ui->listWidget_nodes_directories->clear();

    for(int l = 0;l<ui->treeWidget->topLevelItemCount();l++){
        QTreeWidgetItem *item = ui->treeWidget->topLevelItem(l);
        ui->listWidget_nodes_directories->addItem(item->text(1)+","+item->text(0));
        int index = item->text(0).toInt();
        // shift go back one with indices
        index > removed ? item->setText(0,QString::number(index-1)) : item->setText(0,item->text(0));
   }

    for(int l = 0;l<ui->treeWidget->topLevelItemCount();l++){
        QTreeWidgetItem *item = ui->treeWidget->topLevelItem(l);
        for(int k = 0; k<item->childCount();k++){

            QTreeWidgetItem *item_child = item->child(k);
            int index_child = item_child->text(0).toInt();
            // go back one with indices
            index_child > removed? item_child->setText(0,QString::number(index_child-1)):item_child->setText(0,QString::number(index_child));
        }

        //const int child_cont = item->childCount();
        for(int k = 0; k<item->childCount();k++){



            QTreeWidgetItem *item_child = item->child(k);
            int index_child = item_child->text(0).toInt();

            // go back one with indices
            index_child > removed? item_child->setText(0,QString::number(index_child-1)):item_child->setText(0,QString::number(index_child));
            if(index_child==removed){delete item_child; k--;}

        }
    }

}

void UserInterfaceCreator::load_interface(QString filename){
    // load the text file
    QFile textfile(filename);

    // open the text file
    textfile.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream ascread(&textfile);

    if(textfile.isOpen()){
        // read each line of the file
        QString line = ascread.readLine();

        while(!line.isNull()){
            // break the line up in usable parts
            QStringList list = line.split(",");

            // check the type of line
            /* n-> node
             * j-> join
             */
            if(list[0]=="n"){
                QString name ="";
                int index = 0, add = 0;
                name=list[5];
                QTextStream(&list[6])>>add;
                QTextStream(&list[1])>>index;
                if(add==1){
                    ui->listWidget_nodes_directories->addItem(name+","+QString::number(index));
                }

            }
            // read next line
           line = ascread.readLine();
        }

        // close the textfile
        textfile.close();
    }
}

void UserInterfaceCreator::AddAfterCurrentIndex(){
  // qDebug()<< ui->treeWidget->selectedItems().count();

  ui->listWidget_nodes_directories->addItem("DIR:"+ui->lineEdit_new_directory->text()+","+QString::number(ui->treeWidget->topLevelItemCount()));
  addTreeRoot(QString::number(ui->treeWidget->topLevelItemCount()),"DIR:"+ui->lineEdit_new_directory->text());
}

void UserInterfaceCreator::on_pushButton_add_child_clicked(){

    if(ui->listWidget_nodes_directories->selectedItems().count()>0){
        QTreeWidgetItem *item = ui->treeWidget->currentItem();
        if(!item) return;
        QStringList ls =ui->listWidget_nodes_directories->currentItem()->text().split(",");
        addTreeChild(item,ls.value(1),ls.value(0));
    }
}

void UserInterfaceCreator::save_to_file(QString){

}
