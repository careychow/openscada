
//OpenSCADA system module UI.QTCfg file: selfwidg.cpp
/***************************************************************************
 *   Copyright (C) 2004-2007 by Roman Savochenko                           *
 *   rom_as@fromru.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 2 of the License.               *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QTextEdit>
#include <QItemEditorFactory>
#include <QMetaProperty>

#include <tsys.h>

#include "tuimod.h"
#include "selfwidg.h"

using namespace QTCFG;

//*************************************************
//* ImgView: Image view widget.                   *
//*************************************************
ImgView::ImgView( QWidget * parent, Qt::WindowFlags f, int ih_sz, int iv_sz ) :
    QWidget(parent,f), h_sz(ih_sz), v_sz(iv_sz)
{

}

ImgView::~ImgView( )
{

}

bool ImgView::setImage( const string &imgdata )
{
    bool rez = m_img.loadFromData((const uchar*)imgdata.c_str(),imgdata.size());

    if(rez)
    {
	m_img = m_img.scaled(QSize(h_sz?vmin(h_sz,m_img.width()):m_img.width(),
				   v_sz?vmin(v_sz,m_img.height()):m_img.height()),Qt::KeepAspectRatio);
	setMinimumSize(m_img.width(),m_img.height());
    }
    else
    {
	m_img = QImage();
	setMinimumSize(200,40);
    }

    update();

    return rez;
}

void ImgView::paintEvent( QPaintEvent * )
{
    QPainter pnt( this );
    if(m_img.isNull())
    {
	pnt.setWindow(0,0,rect().width(),rect().height());
	pnt.setPen(QColor(255,0,0));
	pnt.setBackground(QBrush(QColor(210,237,234)));
	pnt.drawRect(0,0,199,39);
	pnt.drawText(3,3,194,34,Qt::AlignCenter,_("Picture no set!"));
    }
    else
    {
	pnt.setWindow( 0, 0, rect().width(), rect().height() );
	pnt.drawImage(QPoint(0,0),m_img);
	pnt.setPen(QColor(0,0,255));
	pnt.drawRect( 0, 0, m_img.width()-1, m_img.height()-1 );
    }
}

//*************************************************
//* LineEdit: Line edit widget.                   *
//*************************************************
LineEdit::LineEdit( QWidget *parent, bool prev_dis ) :
    QWidget( parent ), bt_fld(NULL)
{
    QHBoxLayout *box = new QHBoxLayout(this);
    box->setMargin(0);
    box->setSpacing(0);

    ed_fld = new QLineEdit(this);
    connect( ed_fld, SIGNAL( textEdited(const QString&) ), SLOT( changed(const QString&) ) );
    box->addWidget(ed_fld);

    if( !prev_dis )
    {
	bt_fld = new QPushButton(this);
	bt_fld->setIcon(QIcon(":/images/ok.png"));
	bt_fld->setIconSize(QSize(12,12));
	bt_fld->setSizePolicy( QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) );
	bt_fld->setEnabled(false);
	bt_fld->setVisible(false);
	connect( bt_fld, SIGNAL( released() ), this, SLOT( applySlot() ) );
	box->addWidget(bt_fld);
    }
}

bool LineEdit::isChanged( )
{
    if( bt_fld && bt_fld->isEnabled() )	return true;
    return false;
}

bool LineEdit::hasFocus( ) const
{
    return ed_fld->hasFocus( );
}

void LineEdit::changed( const QString& str )
{
    if( bt_fld && !bt_fld->isEnabled() )
    {
	bt_fld->setEnabled(true);
	bt_fld->setVisible(true);
    }
    emit textChanged(str);
}

void LineEdit::setText(const QString &txt)
{
    ed_fld->setText(txt);
    ed_fld->setCursorPosition(0);
    if( bt_fld && bt_fld->isEnabled() )
    {
	bt_fld->setEnabled(false);
	bt_fld->setVisible(false);
    }
}

QString LineEdit::text() const
{
    return ed_fld->text();
}

void LineEdit::applySlot( )
{
    emit textChanged(text());

    bt_fld->setEnabled(false);
    bt_fld->setVisible(false);

    emit apply();
}

bool LineEdit::event( QEvent * e )
{
    if( e->type() == QEvent::KeyRelease && bt_fld && bt_fld->isEnabled() )
    {
	QKeyEvent *keyEvent = (QKeyEvent *)e;
	if(keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
	{
	    bt_fld->animateClick( );
	    return true;
	}
	else if(keyEvent->key() == Qt::Key_Escape )
	{
	    bt_fld->setEnabled(false);
	    bt_fld->setVisible(false);
	    emit cancel();
	    return true;
	}
    }
    return QWidget::event(e);
}

//*************************************************
//* TextEdit: Text edit widget.                   *
//*************************************************
TextEdit::TextEdit( QWidget *parent, const char *name, bool prev_dis ) :
    QWidget(parent), but_box(NULL), isInit(false)
{
    setObjectName(name);
    QVBoxLayout *box = new QVBoxLayout(this);
    box->setMargin(0);
    box->setSpacing(0);

    ed_fld = new QTextEdit(this);
    connect( ed_fld, SIGNAL( textChanged() ), this, SLOT( changed() ) );
    box->addWidget(ed_fld);

    if( !prev_dis )
    {
	but_box = new QDialogButtonBox( QDialogButtonBox::Apply|
					QDialogButtonBox::Cancel, Qt::Horizontal, this );
	QImage ico_t;
	but_box->button(QDialogButtonBox::Apply)->setText(_("Apply"));
	if(!ico_t.load(TUIS::icoPath("button_ok").c_str())) ico_t.load(":/images/button_ok.png");
	but_box->button(QDialogButtonBox::Apply)->setIcon(QPixmap::fromImage(ico_t));
	connect(but_box->button(QDialogButtonBox::Apply), SIGNAL(clicked()), this, SLOT(btApply()));
	but_box->button(QDialogButtonBox::Cancel)->setText(_("Cancel"));
	if(!ico_t.load(TUIS::icoPath("button_cancel").c_str())) ico_t.load(":/images/button_cancel.png");
	but_box->button(QDialogButtonBox::Cancel)->setIcon(QPixmap::fromImage(ico_t));
	connect(but_box, SIGNAL(rejected()), this, SLOT(btCancel()));
	but_box->setVisible(false);
	box->addWidget(but_box);
    }
}

bool TextEdit::isChanged( )
{
    if( but_box && but_box->isVisible() ) return true;
    return false;
}

QString TextEdit::text()
{
    return ed_fld->toPlainText();
}

bool TextEdit::hasFocus( ) const
{
    return ed_fld->hasFocus( );
}

void TextEdit::setText(const QString &text)
{
    isInit=true;
    ed_fld->setPlainText(text);
    ed_fld->document()->setModified(false);
    isInit=false;
    changed();
}

void TextEdit::changed()
{
    if( isInit ) return;
    if( but_box ) but_box->setVisible(ed_fld->document()->isModified());
    emit textChanged(text());
}

void TextEdit::btApply( )
{
    emit textChanged(text());
    but_box->setVisible(false);
    emit apply();
}

void TextEdit::btCancel( )
{
    but_box->setVisible(false);
    emit cancel();
}


//*************************************************
//* DateTimeEdit: Data and time edit widget.      *
//*************************************************
DateTimeEdit::DateTimeEdit( QWidget *parent, bool prev_dis ) : 
    QWidget( parent ), bt_fld(NULL)
{
    QHBoxLayout *box = new QHBoxLayout(this);
    box->setMargin(0);
    box->setSpacing(0);

    ed_fld = new QDateTimeEdit(this);
    ed_fld->setDisplayFormat("dd.MM.yyyy hh:mm:ss");
    connect( ed_fld, SIGNAL( dateTimeChanged(const QDateTime&) ), this, SLOT( changed(const QDateTime&) ) );
    box->addWidget(ed_fld);
    if( !prev_dis )
    {
	bt_fld = new QPushButton(this);
	bt_fld->setIcon(QIcon(":/images/ok.png"));
	bt_fld->setIconSize(QSize(12,12));
	bt_fld->setSizePolicy( QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) );
	bt_fld->setEnabled(false);
	bt_fld->setVisible(false);
	connect( bt_fld, SIGNAL( released() ), this, SLOT( applySlot() ) );
	box->addWidget(bt_fld);
    }
}

bool DateTimeEdit::isChanged( )
{
    if( bt_fld && bt_fld->isEnabled() ) return true;
    return false;
}

bool DateTimeEdit::hasFocus( ) const
{
    return ed_fld->hasFocus( );
}

void DateTimeEdit::changed( const QDateTime& dt )
{
    if( bt_fld && !bt_fld->isEnabled() )
    {
	bt_fld->setEnabled(true);
	bt_fld->setVisible(true);
    }
    emit valueChanged(dt);
}

void DateTimeEdit::setDateTime ( const QDateTime & dt )
{
    ed_fld->setDateTime(dt);
    if( bt_fld && bt_fld->isEnabled() )
    {
	bt_fld->setEnabled(false);
	bt_fld->setVisible(false);
    }
}

QDateTime DateTimeEdit::dateTime() const
{
    return ed_fld->dateTime();
}

void DateTimeEdit::applySlot( )
{
    emit valueChanged(dateTime());
    bt_fld->setEnabled(false);
    bt_fld->setVisible(false);
    emit apply();
}

bool DateTimeEdit::event( QEvent * e )
{
    if(e->type() == QEvent::KeyRelease && bt_fld && bt_fld->isEnabled())
    {
	QKeyEvent *keyEvent = (QKeyEvent *)e;
	if( keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return )
	{
	    bt_fld->animateClick( );
	    return true;
	}
	else if(keyEvent->key() == Qt::Key_Escape )
	{
	    bt_fld->setEnabled(false);
	    bt_fld->setVisible(false);
	    emit cancel();
	    return true;
	}
    }
    return QWidget::event(e);
}

//*************************************************
//* InputDlg: Id and name input dialog.           *
//*************************************************
InputDlg::InputDlg( QWidget *parent, const QIcon &icon, const QString &mess,
        const QString &ndlg, bool with_id, bool with_nm ) :
		QDialog(parent), m_id(NULL), m_name(NULL)
{
    setWindowTitle(ndlg);
    setMinimumSize( QSize( 120, 150 ) );
    setWindowIcon(icon);
    setSizeGripEnabled(true);

    QVBoxLayout *dlg_lay = new QVBoxLayout(this);
    dlg_lay->setMargin(10);
    dlg_lay->setSpacing(6);

    //- Icon label and text message -
    QHBoxLayout *intr_lay = new QHBoxLayout;
    intr_lay->setSpacing(6);

    QLabel *icon_lab = new QLabel(this);
    icon_lab->setSizePolicy( QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum) );
    icon_lab->setPixmap(icon.pixmap(48));
    intr_lay->addWidget(icon_lab);

    inp_lab = new QLabel(mess,this);
    //inp_lab->setAlignment(Qt::AlignHCenter);
    inp_lab->setWordWrap(true);
    intr_lay->addWidget(inp_lab);
    dlg_lay->addItem(intr_lay);

    //- Id and name fields -
    ed_lay = new QGridLayout;
    if( with_nm || with_id )
    {
	ed_lay->setSpacing(6);
	if( with_id )
	{
	    ed_lay->addWidget( new QLabel(_("ID:"),this), 3, 0 );
	    m_id = new QLineEdit(this);
	    ed_lay->addWidget( m_id, 3, 1 );
	}
	if( with_nm )
	{
	    ed_lay->addWidget( new QLabel(_("Name:"),this), 4, 0 );
	    m_name = new QLineEdit(this);
	    ed_lay->addWidget( m_name, 4, 1 );
	}
    }
    dlg_lay->addItem(ed_lay);

    //- Qk and Cancel buttons -
    dlg_lay->addItem( new QSpacerItem( 10, 0, QSizePolicy::Minimum, QSizePolicy::Expanding ) );

    QFrame *sep = new QFrame(this);
    sep->setFrameShape( QFrame::HLine );
    sep->setFrameShadow( QFrame::Raised );
    dlg_lay->addWidget( sep );

    QDialogButtonBox *but_box = new QDialogButtonBox( QDialogButtonBox::Ok|
						      QDialogButtonBox::Cancel, Qt::Horizontal, this );
    QImage ico_t;
    but_box->button(QDialogButtonBox::Ok)->setText(_("Ok"));
    if(!ico_t.load(TUIS::icoPath("button_ok").c_str())) ico_t.load(":/images/button_ok.png");
    but_box->button(QDialogButtonBox::Ok)->setIcon(QPixmap::fromImage(ico_t));
    connect(but_box, SIGNAL(accepted()), this, SLOT(accept()));
    but_box->button(QDialogButtonBox::Cancel)->setText(_("Cancel"));
    if(!ico_t.load(TUIS::icoPath("button_cancel").c_str())) ico_t.load(":/images/button_cancel.png");
    but_box->button(QDialogButtonBox::Cancel)->setIcon(QPixmap::fromImage(ico_t));
    connect(but_box, SIGNAL(rejected()), this, SLOT(reject()));
    dlg_lay->addWidget( but_box );

    resize(400,120+(40*with_nm)+(40*with_id));
}

QString InputDlg::id()
{
    if( m_id )  return m_id->text();
    return "";
}

QString InputDlg::name()
{
    if( m_name )return m_name->text();
    return "";
}

QString InputDlg::mess( )
{
    return inp_lab->text();
}

void InputDlg::setId(const QString &val)
{
    if( m_id )  m_id->setText(val);
}

void InputDlg::setName(const QString &val)
{
    if( m_name )m_name->setText(val);
}

void InputDlg::setMess( const QString &val )
{
    inp_lab->setText( val );
}

//*************************************************
//* DlgUser: User select dialog                   *
//*************************************************
DlgUser::DlgUser( QWidget *parent ) : QDialog(parent)
{
    setWindowTitle(_("Select user"));

    QVBoxLayout *dlg_lay = new QVBoxLayout(this);
    dlg_lay->setMargin(10);
    dlg_lay->setSpacing(6);

    QGridLayout *ed_lay = new QGridLayout;
    ed_lay->setSpacing(6);
    ed_lay->addWidget( new QLabel(_("User:"),this), 0, 0 );
    users = new QComboBox(this);
    ed_lay->addWidget( users, 0, 1 );
    ed_lay->addWidget( new QLabel(_("Password:"),this), 1, 0 );
    passwd = new QLineEdit(this);
    passwd->setEchoMode( QLineEdit::Password );
    ed_lay->addWidget( passwd, 1, 1 );
    dlg_lay->addItem(ed_lay);

    dlg_lay->addItem( new QSpacerItem( 20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding ) );

    QFrame *sep = new QFrame(this);
    sep->setFrameShape( QFrame::HLine );
    sep->setFrameShadow( QFrame::Raised );
    dlg_lay->addWidget( sep );

    QDialogButtonBox *but_box = new QDialogButtonBox( QDialogButtonBox::Ok|
						      QDialogButtonBox::Cancel, Qt::Horizontal, this );
    QImage ico_t;
    but_box->button(QDialogButtonBox::Ok)->setText(_("Ok"));
    if(!ico_t.load(TUIS::icoPath("button_ok").c_str())) ico_t.load(":/images/button_ok.png");
    but_box->button(QDialogButtonBox::Ok)->setIcon(QPixmap::fromImage(ico_t));
    connect(but_box, SIGNAL(accepted()), this, SLOT(accept()));
    but_box->button(QDialogButtonBox::Cancel)->setText(_("Cancel"));
    if(!ico_t.load(TUIS::icoPath("button_cancel").c_str())) ico_t.load(":/images/button_cancel.png");
    but_box->button(QDialogButtonBox::Cancel)->setIcon(QPixmap::fromImage(ico_t));
    connect(but_box, SIGNAL(rejected()), this, SLOT(reject()));
    dlg_lay->addWidget( but_box );

    connect(this, SIGNAL(finished(int)), this, SLOT(finish(int)));

    //- Fill users list -
    vector<string> u_list;
    SYS->security().at().usrList(u_list);
    for(int i_l = 0; i_l < u_list.size(); i_l++ )
    {
	string simg = TSYS::strDecode(SYS->security().at().usrAt(u_list[i_l]).at().picture(),TSYS::base64);
	QImage img;
	if( img.loadFromData((const uchar*)simg.c_str(),simg.size()) )
	    users->addItem(QPixmap::fromImage(img),u_list[i_l].c_str());
	else users->addItem(u_list[i_l].c_str());
    }
}

QString DlgUser::user()
{
    return users->currentText();
}

QString DlgUser::password()
{
    return passwd->text();
}

void DlgUser::finish( int result )
{
    if( result )
    {
	//- Check user -
	if( SYS->security().at().usrPresent(user().toAscii().data()) &&
		SYS->security().at().usrAt(user().toAscii().data()).at().auth(password().toAscii().data()) )
	    setResult(SelOK);
	else setResult(SelErr);
    }
    else setResult(SelCancel);
}

//*********************************************
//* UserStBar: Status bar user widget.        *
//*********************************************
UserStBar::UserStBar( const QString &iuser, QWidget * parent ) : QLabel(parent)
{
    setUser(iuser);
}

QString UserStBar::user( )
{
    return user_txt;
}

void UserStBar::setUser( const QString &val )
{
    setText(QString("<font color='%1'>%2</font>").arg((val=="root")?"red":"green").arg(val));
    user_txt = val;
}

bool UserStBar::event( QEvent *event )
{
    if( event->type() == QEvent::MouseButtonDblClick )  userSel();
    return QLabel::event( event );
}

bool UserStBar::userSel( )
{
    DlgUser d_usr(parentWidget());
    int rez = d_usr.exec();
    if( rez == DlgUser::SelOK && d_usr.user() != user() )
    {
	setUser( d_usr.user() );
	emit userChanged();
	return true;
    }
    else if( rez == DlgUser::SelErr )
        mod->postMess(mod->nodePath().c_str(),_("Auth wrong!!!"),TUIMod::Warning,this);

    return false;
}

//*************************************************
//* TableDelegate: Combobox table delegate.       *
//*************************************************
TableDelegate::TableDelegate(QObject *parent) : QItemDelegate(parent)
{

}

void TableDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    drawFocus(painter,option,option.rect.adjusted(+1,+1,-1,-1));

    QVariant value = index.data(Qt::DisplayRole);
    switch(value.type())
    {
	case QVariant::Bool:
	    //painter->save();
	    if(value.toBool())
	    {
		QImage img(":/images/ok.png");
		painter->drawImage(option.rect.center().x()-img.width()/2,option.rect.center().y()-img.height()/2,img);
	    }
	    //painter->restore();
	    break;
	default:
	    drawDisplay(painter,option,option.rect,value.toString());
	    break;
    }
}

QWidget *TableDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QWidget *w_del;
    if(!index.isValid()) return 0;

    QVariant value = index.data(Qt::DisplayRole);
    QVariant val_user = index.data(Qt::UserRole);

    if(val_user.isValid()) w_del = new QComboBox(parent);
    else
    {
	QItemEditorFactory factory;
	w_del = factory.createEditor(value.type(), parent);
    }
    w_del->installEventFilter(const_cast<TableDelegate*>(this));
    return w_del;
}

void TableDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QVariant value = index.data(Qt::DisplayRole);
    QVariant val_user = index.data(Qt::UserRole);

    if(dynamic_cast<QComboBox*>(editor))
    {
	QComboBox *comb = dynamic_cast<QComboBox*>(editor);
	if(value.type() == QVariant::Bool)	comb->setCurrentIndex(value.toBool());
	else if(val_user.isValid())
	{
	    comb->addItems(val_user.toStringList());
	    comb->setCurrentIndex(comb->findText(value.toString()));
	}
	return;
    }
    if(dynamic_cast<QLineEdit*>(editor))
    {
	QLineEdit *led = dynamic_cast<QLineEdit*>(editor);
	led->setText(value.toString());
	return;
    }
}

void TableDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    if(dynamic_cast<QComboBox*>(editor))
    {
	QComboBox *comb = dynamic_cast<QComboBox*>(editor);
	QVariant val_user = index.data(Qt::UserRole);
	if(!val_user.isValid())
	    model->setData(index,(bool)comb->currentIndex(),Qt::EditRole);
	else model->setData(index,comb->currentText(),Qt::EditRole);
	return;
    }
    if(dynamic_cast<QLineEdit*>(editor))
    {
	QLineEdit *led = dynamic_cast<QLineEdit*>(editor);
	model->setData(index,led->text(),Qt::EditRole);
	return;
    }
}

void TableDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const
{
    editor->setGeometry(option.rect);
}

bool TableDelegate::eventFilter(QObject *object, QEvent *event)
{
    if(dynamic_cast<QComboBox*>(object))
    {
	QComboBox *comb = dynamic_cast<QComboBox*>(object);
	if(event->type() == QEvent::KeyRelease)
	    switch (static_cast<QKeyEvent *>(event)->key()) 
	    {
		case Qt::Key_Enter:
		case Qt::Key_Return:
		    emit commitData(comb);
		    emit closeEditor(comb, QAbstractItemDelegate::SubmitModelCache);
		    return true;
		case Qt::Key_Escape:
		    emit closeEditor(comb, QAbstractItemDelegate::RevertModelCache);
		    return true;
		default:
		    return false;
	    }
    }

    return QItemDelegate::eventFilter(object,event);
}
