drop database if exists tweet_system;

create database tweet_system;

use tweet_system;

create table user(
    id BIGINT unsigned not null auto_increment primary key,
    role TEXT not null,
    name VARCHAR(40) not null unique,
    nickname TEXT not null,
    password TEXT not null,
    reg_time DATETIME not null,
    sex TEXT not null,
    age INT,
    email TEXT not null,
    introduction TEXT not null
) engine=innodb;

create table post(
    id BIGINT unsigned not null auto_increment primary key,
    deleted BOOL not null,
    user_id BIGINT unsigned not null,
    publish_time DATETIME not null,
    content TEXT not null,
    audit_state TEXT not null,
    reply_post_id BIGINT unsigned,
    forward_post_id BIGINT unsigned,
    foreign key(user_id) references user(id),
    foreign key(reply_post_id) references post(id),
    foreign key(forward_post_id) references post(id)
) engine=innodb;

create table Tlike(
    id BIGINT unsigned not null auto_increment primary key,
    user_id BIGINT unsigned not null,
    post_id BIGINT unsigned not null,
    time DATETIME not null,
    foreign key(user_id) references user(id),
    foreign key(post_id) references post(id)
) engine=innodb;

create table collection(
    id BIGINT unsigned not null auto_increment primary key,
    user_id BIGINT unsigned not null,
    post_id BIGINT unsigned not null,
    time DATETIME not null,
    foreign key(user_id) references user(id),
    foreign key(post_id) references post(id)
) engine=innodb;

create table user_relationship(
    id BIGINT unsigned not null auto_increment primary key,
    user_id BIGINT unsigned not null,
    target_user_id BIGINT unsigned not null,
    block BOOL not null,
    foreign key(user_id) references user(id),
    foreign key(target_user_id) references user(id)
) engine=innodb;

create table post_recommendation(
    id BIGINT unsigned not null auto_increment primary key,
    user_id BIGINT unsigned not null,
    foreign key(user_id) references user(id)
) engine=innodb;
