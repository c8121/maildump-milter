/*
 Archive Metdatabase DDL
 
 The archive database structure is designed generically.
 As the archive takes any type of file, the database stores generic metadata.
 
 Each entry is identified by HASH value, created from file content.
 Each entry has one or more ORIGINs, which can be a sender-email-address or the original file path.
 Each entry also has one or more OWNERS, which can be recipient-email-address or a user name .
 
*/

CREATE DATABASE archive;
USE archive;

/* 
 Example how to create a user:
 CREATE USER 'archive'@'localhost' IDENTIFIED BY '<insert-password-here>';
 GRANT ALL PRIVILEGES ON archive.* to 'archive'@'localhost';
*/

CREATE TABLE `OWNER` (
	`ID` int(11) NOT NULL AUTO_INCREMENT,
	`NAME` varchar(254) DEFAULT NULL,
	PRIMARY KEY (`ID`)
) ENGINE MyISAM DEFAULT CHARSET=utf8mb4;

CREATE TABLE `ORIGIN` (
	`ID` int(11) NOT NULL AUTO_INCREMENT,
	`NAME` text DEFAULT NULL,
	PRIMARY KEY (`ID`)
) ENGINE MyISAM DEFAULT CHARSET=utf8mb4;

CREATE TABLE `ENTRY` (
	`ID` int(11) NOT NULL AUTO_INCREMENT,
	`HASH` char(64) NOT NULL UNIQUE,
	`NAME` varchar(254) DEFAULT NULL,
	`ARRIVED` datetime DEFAULT NULL,
	PRIMARY KEY (`ID`),
	KEY `IDX_ENTRY_HASH` (`HASH`)
) ENGINE MyISAM DEFAULT CHARSET=utf8mb4;

CREATE TABLE `ENTRY_ORIGIN` (
	`ID` int(11) NOT NULL AUTO_INCREMENT,
	`ENTRY` int(11) NOT NULL,
	`OWNER` int(11) DEFAULT NULL,
	`ORIGIN` int(11) DEFAULT NULL,
	`CTIME` datetime DEFAULT NULL,
	`MTIME` datetime DEFAULT NULL,
	PRIMARY KEY (`ID`),
	KEY `IDX_ENTRY_ORIGIN_ENTRY` (`ENTRY`),
	KEY `IDX_ENTRY_ORIGIN_OWNER` (`OWNER`),
	KEY `IDX_ENTRY_ORIGIN_ORIGIN` (`ORIGIN`)
) ENGINE MyISAM DEFAULT CHARSET=utf8mb4;

