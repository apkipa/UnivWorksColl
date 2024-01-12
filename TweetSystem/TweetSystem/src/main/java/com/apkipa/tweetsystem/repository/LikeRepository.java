package com.apkipa.tweetsystem.repository;

import com.apkipa.tweetsystem.model.Like;
import org.babyfish.jimmer.spring.repository.JRepository;

import java.util.Optional;

public interface LikeRepository extends JRepository<Like, Long> {
    Optional<Like> findByUserIdAndPostId(Long userId, Long postId);
}
