package com.apkipa.tweetsystem.model;

import org.babyfish.jimmer.sql.*;
import org.jetbrains.annotations.Nullable;

import java.sql.Timestamp;
import java.time.Instant;

@Entity
public interface Collection {
    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    long id();

    @ManyToOne
    User user();
    @Nullable
    @ManyToOne
    Post post();
    Timestamp time();
}
