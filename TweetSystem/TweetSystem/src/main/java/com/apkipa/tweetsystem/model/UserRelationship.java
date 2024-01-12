package com.apkipa.tweetsystem.model;

import org.babyfish.jimmer.sql.*;

@Entity
public interface UserRelationship {
    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    long id();

    @ManyToOne
    User user();
    @ManyToOne
    User targetUser();
    boolean isBlock();
}
