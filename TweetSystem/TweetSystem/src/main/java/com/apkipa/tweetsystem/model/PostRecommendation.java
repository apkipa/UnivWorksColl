package com.apkipa.tweetsystem.model;

import org.babyfish.jimmer.sql.*;

@Entity
public interface PostRecommendation {
    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    long id();

    @OneToOne
    User user();
}
