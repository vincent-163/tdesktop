/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "intro/intro_start.h"

#include "boxes/abstract_box.h"
#include "core/application.h"
#include "lang/lang_keys.h"
#include "intro/intro_qr.h"
#include "intro/intro_phone.h"
#include "mtproto/mtproto_config.h"
#include "mtproto/mtproto_dc_options.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/layers/generic_box.h"
#include "main/main_account.h"
#include "main/main_app_config.h"
#include "styles/style_intro.h"

namespace Intro {
namespace details {

StartWidget::StartWidget(
	QWidget *parent,
	not_null<Main::Account*> account,
	not_null<Data*> data)
: Step(parent, account, data, true) {
	setMouseTracking(true);
	setTitleText(rpl::single(u"Telegram Desktop"_q));
	setDescriptionText(tr::lng_intro_about());
	show();

	_privateServer = Ui::CreateChild<Ui::LinkButton>(
		this,
		tr::lng_private_server_link(tr::now));
	_privateServer->show();
	rpl::combine(
		sizeValue(),
		_privateServer->widthValue()
	) | rpl::on_next([=](QSize size, int linkWidth) {
		_privateServer->moveToLeft(
			(size.width() - linkWidth) / 2,
			contentTop()
				+ st::introDescriptionTop
				+ 2 * st::normalFont->height
				+ st::introLinkTop);
	}, _privateServer->lifetime());
	_privateServer->setClickedCallback([=] {
		showPrivateServerBox();
	});
}

void StartWidget::showPrivateServerBox() {
	const auto config = MTP::LoadPrivateServerConfig();
	Ui::show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(tr::lng_private_server_title(tr::now));

		const auto address = box->addRow(
			object_ptr<Ui::InputField>(
				box,
				st::defaultInputField,
				tr::lng_private_server_address(),
				config.address));
		const auto port = box->addRow(
			object_ptr<Ui::InputField>(
				box,
				st::defaultInputField,
				tr::lng_private_server_port(),
				QString::number(config.port)));
		const auto key = box->addRow(
			object_ptr<Ui::InputField>(
				box,
				st::defaultInputField,
				Ui::InputField::Mode::MultiLine,
				tr::lng_private_server_key(),
				config.publicKey));
		key->setMinHeight(st::normalFont->height * 6);
		box->setFocusCallback([=] { address->setFocusFast(); });

		const auto submit = [=] {
			auto next = MTP::PrivateServerConfig();
			next.enabled = true;
			next.address = address->getLastText().trimmed();
			next.port = port->getLastText().trimmed().toInt();
			next.publicKey = key->getLastText().trimmed();
			if (next.address.isEmpty()
				|| next.port < 1
				|| next.port > 65535
				|| !next.publicKey.contains(u"-----BEGIN"_q)
				|| !next.publicKey.contains(u"-----END"_q)
				|| !account().mtp().dcOptions().applyPrivateServer(next)) {
				key->showError();
				box->uiShow()->showToast(
					tr::lng_private_server_invalid(tr::now));
				return;
			}
			MTP::SavePrivateServerConfig(next);
			Core::App().fallbackProductionConfig().dcOptions(
			).applyPrivateServer(next);
			box->closeBox();
		};
		key->submits(
		) | rpl::on_next([=](auto) { submit(); }, key->lifetime());
		box->addButton(tr::lng_settings_save(), submit);
		box->addLeftButton(tr::lng_private_server_clear(), [=] {
			auto next = MTP::PrivateServerConfig();
			MTP::SavePrivateServerConfig(next);
			account().mtp().dcOptions().applyPrivateServer(next);
			Core::App().fallbackProductionConfig().dcOptions(
			).applyPrivateServer(next);
			box->closeBox();
		});
		box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
	}));
}

void StartWidget::submit() {
	account().destroyStaleAuthorizationKeys();
	goNext<QrWidget>();
}

rpl::producer<QString> StartWidget::nextButtonText() const {
	return tr::lng_start_msgs();
}

rpl::producer<> StartWidget::nextButtonFocusRequests() const {
	return _nextButtonFocusRequests.events();
}

void StartWidget::activate() {
	Step::activate();
	setInnerFocus();
}

void StartWidget::setInnerFocus() {
	_nextButtonFocusRequests.fire({});
}

} // namespace details
} // namespace Intro
