package com.apkipa.tweetsystem.repository;

import com.apkipa.tweetsystem.model.UserRelationship;
import com.apkipa.tweetsystem.model.UserRelationshipTable;
import org.babyfish.jimmer.spring.repository.JRepository;
import org.babyfish.jimmer.sql.fetcher.Fetcher;

import java.util.List;
import java.util.Optional;

public interface UserRelationshipRepository extends JRepository<UserRelationship, Long> {
    List<UserRelationship> findAllByUserId(Long id);
    List<UserRelationship> findAllByUserId(Long id, Fetcher<UserRelationship> fetcher);
    List<UserRelationship> findAllByTargetUserId(Long id);
    List<UserRelationship> findAllByTargetUserId(Long id, Fetcher<UserRelationship> fetcher);
    UserRelationship findNullableByUserIdAndTargetUserId(Long userId, Long targetUserId);
    Optional<UserRelationship> findByUserIdAndTargetUserId(Long userId, Long targetUserId);

    UserRelationshipTable table = UserRelationshipTable.$;

    default boolean isBlockedByUser(Long thisUserId, Long targetUserId) {
        var relationship = findByUserIdAndTargetUserId(targetUserId, thisUserId);
        if (relationship.isEmpty()) {
            return false;
        }
        return relationship.get().isBlock();
    }
    default List<Long> listAllFollowingIdByUserId(Long userId) {
        return sql()
                .createQuery(table)
                .where(table.user().id().eq(userId))
                .where(table.block().eq(false))
                .select(table.targetUserId())
                .execute();
    }
    default List<Long> listAllBlockedIdByUserId(Long userId) {
        return sql()
                .createQuery(table)
                .where(table.user().id().eq(userId))
                .where(table.block().eq(true))
                .select(table.targetUserId())
                .execute();
    }
}
