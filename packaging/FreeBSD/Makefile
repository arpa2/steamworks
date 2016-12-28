# Created by: Adriaan de Groot <groot@kde.org>
# $FreeBSD: $

PORTNAME=	steamworks
PORTVERSION=	0.91
CATEGORIES=	security net

USE_GITHUB=	yes
GH_ACCOUNT=	arpa2
GH_PROJECT=	${PORTNAME}
GH_TAGNAME=	a37d3c0740612a993c726ab1891ece816ba6fd66

MAINTAINER=	kde@FreeBSD.org
COMMENT=	SteamWorks Configuration-through-LDAP components

USES=		cmake sqlite
LIB_DEPENDS=	liblog4cpp.so:devel/log4cpp \
		liblber.so:net/openldap24-client \
		libldap.so:net/openldap24-client

CMAKE_SOURCE_PATH=	${WRKSRC}/src

.include <bsd.port.mk>

