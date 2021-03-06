#include "editgatedialog.h"
#include "ui_editgatedialog.h"

EditGateDialog::EditGateDialog(QStringList keys, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditGateDialog)
{
    ui->setupUi(this);

    for(int i = 0; i < Gate::Type::COUNT; i++)
        ui->typeComboBox->addItem(GATE_TYPE_STRINGS[i]);

    for(QString key : keys)
        ui->keyList->addItem(key);

    // Prevent lists from being edittable
    ui->keyListAdded->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->keyList->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

EditGateDialog::EditGateDialog(Gate* gate, QStringList keys, QWidget* parent)
    : EditGateDialog(keys, parent)
{
    ui->nameEdit->setText(gate->getName());

    if(gate->isTriggered())
        ui->isTriggeredCheckBox->setChecked(true);
    else
        ui->isTriggeredCheckBox->setChecked(false);

    ui->typeComboBox->setCurrentIndex(gate->getType());

    for(QString string : gate->getKeys())
    {
        ui->keyListAdded->addItem(string);

        // Find and remove the item from other view
        removeKeyFromList(string, ui->keyList);
    }
}

EditGateDialog::~EditGateDialog()
{
    delete ui;
}

void EditGateDialog::on_addKeyButton_clicked()
{
     QVariant selectedKey = ui->keyList->currentIndex().data();
     if(!selectedKey.isNull())
     {
        QString key = selectedKey.toString();
        ui->keyListAdded->addItem(key);

        removeKeyFromList(key, ui->keyList);
     }
     else
         QMessageBox::warning(this, "Error", "Could not add key, no key was selected.", QMessageBox::Ok);
}

void EditGateDialog::on_removeKeyButton_clicked()
{
    QVariant selectedKey = ui->keyListAdded->currentIndex().data();
    if(!selectedKey.isNull())
    {
        QString key = selectedKey.toString();
        ui->keyList->addItem(key);

        removeKeyFromList(key, ui->keyListAdded);
    }
    else
        QMessageBox::warning(this, "Error", "Could not remove key, no key was selected.", QMessageBox::Ok);
}

QList<QListWidgetItem*> EditGateDialog::getAllItems(QListWidget* widget)
{
    QList<QListWidgetItem*> items = QList<QListWidgetItem*>();

    for(int i = 0; i < widget->count(); i++)
        items.append(widget->item(i));

    return items;
}

void EditGateDialog::removeKeyFromList(QString key, QListWidget* widget)
{
    for(int i = 0; i < widget->count(); i++)
    {
        if(widget->item(i)->data(Qt::DisplayRole) == key)
        {
            widget->takeItem(i);
            return;
        }
    }
}

void EditGateDialog::on_okButton_clicked()
{
    QRegularExpression reg = QRegularExpression("^[A-Za-z0-9]+$");
    if(ui->nameEdit->text().length() == 0 || !reg.match(ui->nameEdit->text()).hasMatch())
        QMessageBox::warning(this, "Error", "Please enter a valid event name. Must be more than 0 characters, and contain only letters and numbers",
                             QMessageBox::Ok);
    else
    {
        name = ui->nameEdit->text();
        type = static_cast<Gate::Type>(ui->typeComboBox->currentIndex());
        triggered = ui->isTriggeredCheckBox->isChecked();
        keys = QStringList();

        QList<QListWidgetItem*> items = getAllItems(ui->keyListAdded);
        for(QListWidgetItem* item : items)
            keys << item->data(Qt::DisplayRole).toString();

        accept();
    }

}

void EditGateDialog::on_cancelButton_clicked()
{
    reject();
}
