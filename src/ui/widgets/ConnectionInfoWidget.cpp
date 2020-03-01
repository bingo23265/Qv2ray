#include "ConnectionInfoWidget.hpp"

#include "3rdparty/qzxing/src/QZXing.h"
#include "common/QvHelpers.hpp"
#include "core/CoreUtils.hpp"
#include "core/connection/Serialization.hpp"

ConnectionInfoWidget::ConnectionInfoWidget(QWidget *parent) : QWidget(parent)
{
    setupUi(this);
    duplicateBtn->setIcon(QICON_R("duplicate.png"));
    deleteBtn->setIcon(QICON_R("delete.png"));
    editBtn->setIcon(QICON_R("edit.png"));
    editJsonBtn->setIcon(QICON_R("json.png"));
    //
    shareLinkTxt->setAutoFillBackground(true);
    shareLinkTxt->setCursor(QCursor(Qt::CursorShape::IBeamCursor));
    shareLinkTxt->installEventFilter(this);
    //
    connect(ConnectionManager, &QvConnectionHandler::OnConnected, this, &ConnectionInfoWidget::OnConnected);
    connect(ConnectionManager, &QvConnectionHandler::OnDisconnected, this, &ConnectionInfoWidget::OnDisConnected);
}

void ConnectionInfoWidget::ShowDetails(const tuple<GroupId, ConnectionId> &_identifier)
{
    shareLinkTxt->setStyleSheet("border-bottom: 1px solid gray; border-radius: 0px; padding: 2px; background-color: " +
                                this->palette().color(this->backgroundRole()).name(QColor::HexRgb));
    this->groupId = get<0>(_identifier);
    this->connectionId = get<1>(_identifier);
    bool isConnection = connectionId != NullConnectionId;
    //
    editBtn->setEnabled(isConnection);
    editJsonBtn->setEnabled(isConnection);
    connectBtn->setEnabled(isConnection);
    duplicateBtn->setEnabled(isConnection);

    stackedWidget->setCurrentIndex(isConnection ? 0 : 1);
    if (isConnection)
    {
        groupLabel->setText(ConnectionManager->GetDisplayName(groupId, 175));
        protocolLabel->setText(ConnectionManager->GetConnectionProtocolString(connectionId));
        auto [protocol, host, port] = ConnectionManager->GetConnectionData(connectionId);
        Q_UNUSED(protocol)
        addressLabel->setText(host);
        portLabel->setNum(port);
        //
        auto shareLink = ConvertConfigToString(connectionId);
        shareLinkTxt->setText(shareLink);
        shareLinkTxt->setCursorPosition(0);
        //
        QZXingEncoderConfig conf;
        conf.border = true;
        conf.imageSize = QSize(400, 400);
        auto img = QZXing().encodeData(shareLink, conf);
        qrLabel->setScaledContents(true);
        qrLabel->setPixmap(QPixmap::fromImage(img));
        //
        connectBtn->setIcon(ConnectionManager->IsConnected(connectionId) ? QICON_R("stop.png") : QICON_R("connect.png"));
    }
    else
    {
        connectBtn->setIcon(QICON_R("connect.png"));
        groupNameLabel->setText(ConnectionManager->GetDisplayName(groupId));
        QStringList shareLinks;
        for (auto connection : ConnectionManager->Connections(groupId))
        {
            shareLinks << ConvertConfigToString(connection, false);
        }
        groupShareTxt->setPlainText(shareLinks.join(NEWLINE));
        groupSubsLinkLabel->setText(ConnectionManager->IsSubscription(groupId) ? get<0>(ConnectionManager->GetSubscriptionData(groupId)) :
                                                                                 tr("Not a subscription"));
    }
}

ConnectionInfoWidget::~ConnectionInfoWidget()
{
}

void ConnectionInfoWidget::on_connectBtn_clicked()
{
    if (ConnectionManager->IsConnected(connectionId))
    {
        ConnectionManager->StopConnection();
    }
    else
    {
        ConnectionManager->StartConnection(connectionId);
    }
}

void ConnectionInfoWidget::on_editBtn_clicked()
{
    emit OnEditRequested(connectionId);
}

void ConnectionInfoWidget::on_editJsonBtn_clicked()
{
    emit OnJsonEditRequested(connectionId);
}

void ConnectionInfoWidget::on_deleteBtn_clicked()
{
    if (QvMessageBoxAsk(this, tr("Delete an item"), tr("Are you sure to delete the current item?")) == QMessageBox::Yes)
    {
        if (connectionId != NullConnectionId)
        {
            ConnectionManager->DeleteConnection(connectionId);
        }
        else
        {
            ConnectionManager->DeleteGroup(groupId);
        }
    }
}

bool ConnectionInfoWidget::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease)
    {
        if (shareLinkTxt->underMouse())
        {
            if (!shareLinkTxt->hasSelectedText())
            {
                shareLinkTxt->selectAll();
            }
        }
    }

    return QWidget::eventFilter(object, event);
}

void ConnectionInfoWidget::OnConnected(const ConnectionId &id)
{
    if (connectionId == id)
    {
        connectBtn->setIcon(QICON_R("stop.png"));
    }
}

void ConnectionInfoWidget::OnDisConnected(const ConnectionId &id)
{
    if (connectionId == id)
    {
        connectBtn->setIcon(QICON_R("connect.png"));
    }
}

void ConnectionInfoWidget::on_duplicateBtn_clicked()
{
    if (connectionId != NullConnectionId)
    {
        ConnectionManager->CreateConnection(ConnectionManager->GetDisplayName(connectionId) + tr(" (Copy)"),
                                            ConnectionManager->GetConnectionGroupId(connectionId),
                                            ConnectionManager->GetConnectionRoot(connectionId));
    }
}

void ConnectionInfoWidget::on_latencyBtn_clicked()
{
    if (connectionId != NullConnectionId)
    {
        ConnectionManager->StartLatencyTest(connectionId);
    }
    else
    {
        ConnectionManager->StartLatencyTest(groupId);
    }
}
