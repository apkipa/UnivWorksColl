package com.apkipa.tweetsystem.model;

import org.babyfish.jimmer.sql.*;
import org.jetbrains.annotations.Nullable;

import java.sql.Timestamp;
import java.time.Instant;
import java.util.List;

@Entity
public interface Post {
    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    long id();

    @LogicalDeleted("true")
    boolean isDeleted();

    @ManyToOne
    User user();
    Timestamp publishTime();
    String content();
    EAudit auditState();
    @Nullable
    @ManyToOne
    Post replyPost();
    @Nullable
    @ManyToOne
    Post forwardPost();

    @OneToMany(mappedBy = "post")
    List<Like> likes();

    @OneToMany(mappedBy = "post")
    List<Collection> collections();

    @OneToMany(mappedBy = "replyPost")
    List<Post> replyPosts();
}
