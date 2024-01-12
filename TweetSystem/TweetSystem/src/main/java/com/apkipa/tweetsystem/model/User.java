package com.apkipa.tweetsystem.model;

import org.babyfish.jimmer.sql.*;
import org.jetbrains.annotations.Nullable;

import java.sql.Timestamp;
import java.util.List;

@Entity
public interface User {
    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    long id();

    ERole role();
    String name();
    String nickname();
    String password();
    Timestamp regTime();
    String sex();
    Integer age();
    String email();
    String introduction();

    @Nullable
    @OneToOne(mappedBy = "user")
    PostRecommendation postRecommendation();

    @OneToMany(mappedBy = "user")
    List<UserRelationship> reactiveRelations();
    @OneToMany(mappedBy = "targetUser")
    List<UserRelationship> proactiveRelations();
    @OneToMany(mappedBy = "user")
    List<Post> publishedPosts();
}
