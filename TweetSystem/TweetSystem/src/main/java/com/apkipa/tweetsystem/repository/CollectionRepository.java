package com.apkipa.tweetsystem.repository;

import com.apkipa.tweetsystem.model.Collection;
import org.babyfish.jimmer.spring.repository.JRepository;
import org.babyfish.jimmer.sql.fetcher.Fetcher;

import java.util.List;
import java.util.Optional;

public interface CollectionRepository extends JRepository<Collection, Long> {
    List<Collection> findAllByUserId(Long userId);
    List<Collection> findAllByUserId(Long userId, Fetcher<Collection> fetcher);
    Optional<Collection> findByUserIdAndPostId(Long userId, Long postId);
}
